// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "RemoteSlateServer.h"
#include "InputCore.h"

void FRemoteSlateServer::Tick()
{
	// calculate the time since last tick
	static double LastTickTime = FPlatformTime::Seconds();
	double Now = FPlatformTime::Seconds();
	float DeltaTime = LastTickTime - Now;
	LastTickTime = Now;


	// attempt to open the socket if it needs it
	if (ServerSocket == NULL)
	{
		ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
		if (SocketSubsystem)
		{
			// create a UDP listening socket
			ServerSocket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("UDKRemoteListener"));

			if (ServerSocket)
			{
				// allow reuse (or there will be errors when running multiple game instances on a single machine)
				ServerSocket->SetReuseAddr();

				// bind the socket to listen on the port
				TSharedRef<FInternetAddr> BindAddr = SocketSubsystem->CreateInternetAddr();
				BindAddr->SetAnyAddress();
				int32 Port;
				if (GIsEditor)
				{
					Port = 41766;
					//verify(GConfig->GetInt(TEXT("MobileSupport"), TEXT("UDKRemotePortPIE"), Port, GEngineIni));
				}
				else
				{
					Port = 41765;
					//verify(GConfig->GetInt(TEXT("MobileSupport"), TEXT("UDKRemotePort"), Port, GEngineIni));
				}

				BindAddr->SetPort(Port);
				ServerSocket->Bind(*BindAddr);

				// make an async socket
				ServerSocket->SetNonBlocking(true);

				// create a UDP reply socket
				ReplySocket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("UDKRemoteReply"));
				if (ReplySocket)
				{
					// make an async socket
					ReplySocket->SetNonBlocking(true);
				}

				// 					ImageSocket = GSocketSubsystem->CreateDGramSocket(TEXT("TiltImage"));
				// 					if (ImageSocket)
				// 					{
				// 						// make an async socket
				// 						ImageSocket->SetNonBlocking(TRUE);
				// 					}
			}
		}
	}

	if (ServerSocket && ReplySocket)
	{
		FSlateApplication& CurrentApp = FSlateApplication::Get();

		// try to read some data from the 
		uint8 Msg[256];
		int32 BytesRead;
		while (ServerSocket->RecvFrom(Msg, sizeof(Msg), BytesRead, *ReplyAddr))
		{
			// make sure it's what we expected
			if (BytesRead != sizeof(FMessage))
			{
				UE_LOG(LogSlate, Log, TEXT("Received %d bytes, expected %d"), BytesRead, sizeof(FMessage));
				continue;
			}

			// get the message in a blob we can process
			FMessage* Message = (FMessage*)Msg;

			// make sure tag and version are okay
			bool bIsValidMessageVersion = 
				Message->MagicTag == MESSAGE_MAGIC_ID && 
				// in the future, we can write code to handle older versions
				Message->MessageVersion == CURRENT_MESSAGE_VERSION;

			// make sure we got a later message than last time (handling wrapping
			// around with plenty of slop)
			bool bIsValidID = 
				(Message->DataType == DT_Ping) ||
				(Message->MessageID > HighestMessageReceived) || 
				(Message->MessageID < 1000 && HighestMessageReceived > 65000);
			HighestMessageReceived = Message->MessageID;

			bool bIsValidMessage = bIsValidMessageVersion && bIsValidID;
			if (!bIsValidMessage)
			{
				//					debugf(TEXT("Dropped message %d, %d"), Message->MessageID, Message->DataType);
				continue;
			}
			else
			{
				//					debugf(TEXT("Received message %d, %d"), Message->MessageID, Message->DataType);
			}

			// handle tilt input
			if (Message->DataType == DT_Tilt || Message->DataType == DT_Motion)
			{
				FVector Attitude;
				FVector RotationRate;
				FVector Gravity;
				FVector Accel;

				if (Message->DataType == DT_Tilt)
				{
					// get the raw and processed values from the other end
					FVector CurrentAccelerometer(Message->Data[0], Message->Data[1], Message->Data[2]);
					float Pitch = Message->Data[3], Roll = Message->Data[4];

					// convert it into "Motion" data
					static float LastPitch, LastRoll;

					Attitude = FVector(Pitch,0,Roll);
					RotationRate = FVector(LastPitch - Pitch,0,LastRoll - Roll);
					Gravity = FVector(0,0,0);
					Accel = CurrentAccelerometer;

					// save the previous values to delta rotation
					LastPitch = Pitch;
					LastRoll = Roll;
				}
				else if (Message->DataType == DT_Motion)
				{
					// just use the values directly from UDK Remote
					// Negate the yaw to match direction
					Attitude = FVector(Message->Data[0], -Message->Data[1], Message->Data[2]);
					RotationRate = FVector(Message->Data[3], -Message->Data[4], Message->Data[5]);
					Gravity = FVector(Message->Data[6], Message->Data[7], Message->Data[8]);
					Accel = FVector(Message->Data[9], Message->Data[10], Message->Data[11]);
				}

				// munge the vectors based on the orientation of the remote device
				// 						EUIOrientation Orientation = (EUIOrientation)Message->UIOrientation;
				// 						UMobilePlayerInput::ModifyVectorByOrientation(Attitude, Orientation, TRUE);
				//  						UMobilePlayerInput::ModifyVectorByOrientation(RotationRate, Orientation, TRUE);
				//  						UMobilePlayerInput::ModifyVectorByOrientation(Gravity, Orientation, FALSE);
				//  						UMobilePlayerInput::ModifyVectorByOrientation(Accel, Orientation, FALSE);

				// send the tilt to the input system
				FMotionEvent Event(0, Attitude, RotationRate, Gravity, Accel);
				CurrentApp.OnMotionDetectedMessage(Event);
			}
			else if (Message->DataType == DT_Gyro)
			{
				// ignore for now
			}
			else if (Message->DataType == DT_Ping)
			{
				TimeSinceLastPing = 0.0f;
				// reply
				ReplyAddr->SetPort(41764);

				int32 BytesSent;
				const ANSICHAR* HELO = "HELO";
				ReplySocket->SendTo((uint8*)HELO, 5, BytesSent, *ReplyAddr);
			}
			// handle touches
			else
			{
				// @todo This should be some global value or something in slate??
				static FVector2D LastTouchPositions[EKeys::NUM_TOUCH_KEYS];
					
				if (Message->Handle < EKeys::NUM_TOUCH_KEYS)
				{
					// convert from 0..1 to windows coords to screen coords
					// @todo: Put this into a function in SlateApplication
					if (CurrentApp.GetActiveTopLevelWindow().IsValid())
					{
						FVector2D ScreenPosition;

						// The remote is interested in finding the gameviewport for the user and mapping the input in to it
						TSharedPtr<SViewport> GameViewport = CurrentApp.GetGameViewport();
						if (GameViewport.IsValid())
						{
							FWidgetPath WidgetPath;

							WidgetPath = GameViewportWidgetPath.ToWidgetPath();
						
							if (WidgetPath.Widgets.Num() == 0 || WidgetPath.Widgets.Last().Widget != GameViewport)
							{
								CurrentApp.FindPathToWidget(CurrentApp.GetInteractiveTopLevelWindows(), GameViewport.ToSharedRef(), WidgetPath);
								GameViewportWidgetPath = WidgetPath;
							}

							const FGeometry& GameViewportGeometry = WidgetPath.Widgets.Last().Geometry;
							ScreenPosition = GameViewportGeometry.LocalToAbsolute(FVector2D(Message->Data[0], Message->Data[1]) * GameViewportGeometry.Size);
						}
						else
						{
							const FSlateRect WindowScreenRect = CurrentApp.GetActiveTopLevelWindow()->GetRectInScreen();
							const FVector2D WindowPosition = WindowScreenRect.GetSize() * FVector2D(Message->Data[0], Message->Data[1]);
							ScreenPosition = FVector2D(WindowScreenRect.Left, WindowScreenRect.Top) + WindowPosition;
						}

						// for up/down messages, we need to let the cursor go in the same location (mouse up with
						// a delta confuses things)
						if (Message->DataType == DT_TouchBegan || Message->DataType == DT_TouchEnded)
						{
							LastTouchPositions[Message->Handle] = ScreenPosition;
						}

						// create the event struct
						FPointerEvent Event(0, Message->Handle, ScreenPosition, LastTouchPositions[Message->Handle], Message->DataType != DT_TouchEnded);
						LastTouchPositions[Message->Handle] = ScreenPosition;

						// send input to handler
						if (Message->DataType == DT_TouchBegan)
						{
							CurrentApp.OnTouchStartedMessage( NULL, Event);
						}
						else if (Message->DataType == DT_TouchEnded)
						{
							CurrentApp.OnTouchEndedMessage(Event);
						}
						else
						{
							CurrentApp.OnTouchMovedMessage(Event);
						}
					}
				}
				else
				{
					//checkf(Message->Handle < ARRAY_COUNT(LastTouchPositions), TEXT("Received handle %d, but it's too big for our array"), Message->Handle);
				}
			}
		}

		TimeSinceLastPing += DeltaTime;
	}
}