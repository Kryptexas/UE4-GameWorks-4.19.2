// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "AndroidInputInterface.h"
#include "GenericApplicationMessageHandler.h"
#include <android/input.h>


TArray<TouchInput> FAndroidInputInterface::TouchInputStack = TArray<TouchInput>();
FCriticalSection FAndroidInputInterface::TouchInputCriticalSection;

FAndroidControllerData FAndroidInputInterface::OldControllerData[MAX_NUM_CONTROLLERS];
FAndroidControllerData FAndroidInputInterface::NewControllerData[MAX_NUM_CONTROLLERS];

EControllerButtons::Type FAndroidInputInterface::ButtonMapping[MAX_NUM_CONTROLLER_BUTTONS];
float FAndroidInputInterface::InitialButtonRepeatDelay;
float FAndroidInputInterface::ButtonRepeatDelay;

FDeferredAndroidMessage FAndroidInputInterface::DeferredMessages[MAX_DEFERRED_MESSAGE_QUEUE_SIZE];
int32 FAndroidInputInterface::DeferredMessageQueueLastEntryIndex = 0;
int32 FAndroidInputInterface::DeferredMessageQueueDroppedCount   = 0;


TSharedRef< FAndroidInputInterface > FAndroidInputInterface::Create(  const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler )
{
	return MakeShareable( new FAndroidInputInterface( InMessageHandler ) );
}

FAndroidInputInterface::FAndroidInputInterface( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler )
	: MessageHandler( InMessageHandler )
{
	ButtonMapping[ 0] = EControllerButtons::FaceButtonBottom;
	ButtonMapping[ 1] = EControllerButtons::FaceButtonRight;
	ButtonMapping[ 2] = EControllerButtons::FaceButtonLeft;
	ButtonMapping[ 3] = EControllerButtons::FaceButtonTop;
	ButtonMapping[ 4] = EControllerButtons::LeftShoulder;
	ButtonMapping[ 5] = EControllerButtons::RightShoulder;
	ButtonMapping[ 6] = EControllerButtons::SpecialRight;
	ButtonMapping[ 7] = EControllerButtons::SpecialLeft;
	ButtonMapping[ 8] = EControllerButtons::LeftThumb;
	ButtonMapping[ 9] = EControllerButtons::RightThumb;
	ButtonMapping[10] = EControllerButtons::LeftTriggerThreshold;
	ButtonMapping[11] = EControllerButtons::RightTriggerThreshold;
	ButtonMapping[12] = EControllerButtons::DPadUp;
	ButtonMapping[13] = EControllerButtons::DPadDown;
	ButtonMapping[14] = EControllerButtons::DPadLeft;
	ButtonMapping[15] = EControllerButtons::DPadRight;

	InitialButtonRepeatDelay = 0.2f;
	ButtonRepeatDelay = 0.1f;
}

void FAndroidInputInterface::SetMessageHandler( const TSharedRef< FGenericApplicationMessageHandler >& InMessageHandler )
{
	MessageHandler = InMessageHandler;
}

void FAndroidInputInterface::Tick(float DeltaTime)
{

}

void FAndroidInputInterface::SendControllerEvents()
{
	FScopeLock Lock(&TouchInputCriticalSection);

	for(int i = 0; i < FAndroidInputInterface::TouchInputStack.Num(); ++i)
	{
		TouchInput Touch = FAndroidInputInterface::TouchInputStack[i];

		// send input to handler
		if (Touch.Type == TouchBegan)
		{
			MessageHandler->OnTouchStarted( NULL, Touch.Position, Touch.Handle, 0);
		}
		else if (Touch.Type == TouchEnded)
		{
			MessageHandler->OnTouchEnded(Touch.Position, Touch.Handle, 0);
		}
		else
		{
			MessageHandler->OnTouchMoved(Touch.Position, Touch.Handle, 0);
		}
	}

	// Extract differences in new and old states and send messages
	for (int32 ControllerIndex = 0; ControllerIndex < MAX_NUM_CONTROLLERS; ControllerIndex++)
	{
		FAndroidControllerData& OldControllerState = OldControllerData[ControllerIndex];
		FAndroidControllerData& NewControllerState = NewControllerData[ControllerIndex];

		if (NewControllerState.LXAnalog != OldControllerState.LXAnalog)
		{
			MessageHandler->OnControllerAnalog(EControllerButtons::LeftAnalogX, NewControllerState.DeviceId, NewControllerState.LXAnalog);
		}
		if (NewControllerState.LYAnalog != OldControllerState.LYAnalog)
		{
			//LOGD("    Sending updated LeftAnalogY value of %f", NewControllerState.LYAnalog);
			MessageHandler->OnControllerAnalog(EControllerButtons::LeftAnalogY, NewControllerState.DeviceId, NewControllerState.LYAnalog);
		}
		if (NewControllerState.RXAnalog != OldControllerState.RXAnalog)
		{
			//LOGD("    Sending updated RightAnalogX value of %f", NewControllerState.RXAnalog);
			MessageHandler->OnControllerAnalog(EControllerButtons::RightAnalogX, NewControllerState.DeviceId, NewControllerState.RXAnalog);
		}
		if (NewControllerState.RYAnalog != OldControllerState.RYAnalog)
		{
			//LOGD("    Sending updated RightAnalogY value of %f", NewControllerState.RYAnalog);
			MessageHandler->OnControllerAnalog(EControllerButtons::RightAnalogY, NewControllerState.DeviceId, NewControllerState.RYAnalog);
		}
		if (NewControllerState.LTAnalog != OldControllerState.LTAnalog)
		{
			//LOGD("    Sending updated LeftTriggerAnalog value of %f", NewControllerState.LTAnalog);
			MessageHandler->OnControllerAnalog(EControllerButtons::LeftTriggerAnalog, NewControllerState.DeviceId, NewControllerState.LTAnalog);

			// Handle the trigger theshold "virtual" button state
			//check(ButtonMapping[10] == EControllerButtons::LeftTriggerThreshold);
			NewControllerState.ButtonStates[10] = NewControllerState.LTAnalog >= 0.1f;
		}
		if (NewControllerState.RTAnalog != OldControllerState.RTAnalog)
		{
			//LOGD("    Sending updated RightTriggerAnalog value of %f", NewControllerState.RTAnalog);
			MessageHandler->OnControllerAnalog(EControllerButtons::RightTriggerAnalog, NewControllerState.DeviceId, NewControllerState.RTAnalog);

			// Handle the trigger theshold "virtual" button state
			//check(ButtonMapping[11] == EControllerButtons::RightTriggerThreshold);
			NewControllerState.ButtonStates[11] = NewControllerState.RTAnalog >= 0.1f;
		}

		const double CurrentTime = FPlatformTime::Seconds();

		// For each button check against the previous state and send the correct message if any
		for (int32 ButtonIndex = 0; ButtonIndex < MAX_NUM_CONTROLLER_BUTTONS; ButtonIndex++)
		{
			if (NewControllerState.ButtonStates[ButtonIndex] != OldControllerState.ButtonStates[ButtonIndex])
			{
				if (NewControllerState.ButtonStates[ButtonIndex])
				{
					//LOGD("    Sending joystick button down %d (first)", ButtonMapping[ButtonIndex]);
					MessageHandler->OnControllerButtonPressed(ButtonMapping[ButtonIndex], NewControllerState.DeviceId, false);
				}
				else
				{
					//LOGD("    Sending joystick button up %d", ButtonMapping[ButtonIndex]);
					MessageHandler->OnControllerButtonReleased(ButtonMapping[ButtonIndex], NewControllerState.DeviceId, false);
				}

				if (NewControllerState.ButtonStates[ButtonIndex])
				{
					// This button was pressed - set the button's NextRepeatTime to the InitialButtonRepeatDelay
					NewControllerState.NextRepeatTime[ButtonIndex] = CurrentTime + InitialButtonRepeatDelay;
				}
			}
			else if (NewControllerState.ButtonStates[ButtonIndex] && NewControllerState.NextRepeatTime[ButtonIndex] <= CurrentTime)
			{
				// Send button repeat events
				MessageHandler->OnControllerButtonPressed(ButtonMapping[ButtonIndex], NewControllerState.DeviceId, true);

				// Set the button's NextRepeatTime to the ButtonRepeatDelay
				NewControllerState.NextRepeatTime[ButtonIndex] = CurrentTime + ButtonRepeatDelay;
			}
		}

		// Update the state for next time
		OldControllerState = NewControllerState;
	}

	for (int32 MessageIndex = 0; MessageIndex < FMath::Min(DeferredMessageQueueLastEntryIndex, MAX_DEFERRED_MESSAGE_QUEUE_SIZE); ++MessageIndex)
	{
		const FDeferredAndroidMessage& DeferredMessage = DeferredMessages[MessageIndex];
		
		switch (DeferredMessage.messageType)
		{

			case MessageType_KeyDown:

				MessageHandler->OnKeyDown(DeferredMessage.KeyEventData.keyId, 0, DeferredMessage.KeyEventData.isRepeat);
				MessageHandler->OnKeyChar(DeferredMessage.KeyEventData.unichar,  DeferredMessage.KeyEventData.isRepeat);
				break;

			case MessageType_KeyUp:

				MessageHandler->OnKeyUp(DeferredMessage.KeyEventData.keyId, 0, false);
				break;
		} 
	}

	if (DeferredMessageQueueDroppedCount)
	{
		//should warn that messages got dropped, which message queue?
		DeferredMessageQueueDroppedCount = 0;
	}

	DeferredMessageQueueLastEntryIndex = 0;

	FAndroidInputInterface::TouchInputStack.Empty(0);
}

void FAndroidInputInterface::QueueTouchInput(TArray<TouchInput> InTouchEvents)
{
	FScopeLock Lock(&TouchInputCriticalSection);

	FAndroidInputInterface::TouchInputStack.Append(InTouchEvents);
}

void FAndroidInputInterface::JoystickAxisEvent(int32 deviceId, int32 axisId, float axisValue)
{
	FScopeLock Lock(&TouchInputCriticalSection);

	// For now, force the device to 0
	// Should use Java to enumerate number of controllers and assign device ID
	// to controller number
	deviceId = 0;

	// Apply a small dead zone to the analog sticks
	const float deadZone = 0.2f;
	switch (axisId)
	{
		// Also, invert Y and RZ to match what the engine expects
		case AMOTION_EVENT_AXIS_X:        NewControllerData[deviceId].LXAnalog =  axisValue; break;
		case AMOTION_EVENT_AXIS_Y:        NewControllerData[deviceId].LYAnalog = -axisValue; break;
		case AMOTION_EVENT_AXIS_Z:        NewControllerData[deviceId].RXAnalog =  axisValue; break;
		case AMOTION_EVENT_AXIS_RZ:       NewControllerData[deviceId].RYAnalog = -axisValue; break;
		case AMOTION_EVENT_AXIS_LTRIGGER: NewControllerData[deviceId].LTAnalog =  axisValue; break;
		case AMOTION_EVENT_AXIS_RTRIGGER: NewControllerData[deviceId].RTAnalog =  axisValue; break;
	}
}

void FAndroidInputInterface::JoystickButtonEvent(int32 deviceId, int32 buttonId, bool buttonDown)
{
	FScopeLock Lock(&TouchInputCriticalSection);

	// For now, force the device to 0
	// Should use Java to enumerate number of controllers and assign device ID
	// to controller number
	deviceId = 0;

	switch (buttonId)
	{
		case AKEYCODE_BUTTON_A:      NewControllerData[deviceId].ButtonStates[ 0] = buttonDown; break;
		case AKEYCODE_BUTTON_B:      NewControllerData[deviceId].ButtonStates[ 1] = buttonDown; break;
		case AKEYCODE_BUTTON_X:      NewControllerData[deviceId].ButtonStates[ 2] = buttonDown; break;
		case AKEYCODE_BUTTON_Y:      NewControllerData[deviceId].ButtonStates[ 3] = buttonDown; break;
		case AKEYCODE_BUTTON_L1:     NewControllerData[deviceId].ButtonStates[ 4] = buttonDown; break;
		case AKEYCODE_BUTTON_R1:     NewControllerData[deviceId].ButtonStates[ 5] = buttonDown; break;
		case AKEYCODE_BUTTON_START:  NewControllerData[deviceId].ButtonStates[ 6] = buttonDown; break;
		case AKEYCODE_BUTTON_SELECT: NewControllerData[deviceId].ButtonStates[ 7] = buttonDown; break;
		case AKEYCODE_BUTTON_THUMBL: NewControllerData[deviceId].ButtonStates[ 8] = buttonDown; break;
		case AKEYCODE_BUTTON_THUMBR: NewControllerData[deviceId].ButtonStates[ 9] = buttonDown; break;
		case AKEYCODE_BUTTON_L2:     NewControllerData[deviceId].ButtonStates[10] = buttonDown; break;
		case AKEYCODE_BUTTON_R2:     NewControllerData[deviceId].ButtonStates[11] = buttonDown; break;
		case AKEYCODE_DPAD_UP:       NewControllerData[deviceId].ButtonStates[12] = buttonDown; break;
		case AKEYCODE_DPAD_DOWN:     NewControllerData[deviceId].ButtonStates[13] = buttonDown; break;
		case AKEYCODE_DPAD_LEFT:     NewControllerData[deviceId].ButtonStates[14] = buttonDown; break;
		case AKEYCODE_DPAD_RIGHT:    NewControllerData[deviceId].ButtonStates[15] = buttonDown; break;
	}
}

void FAndroidInputInterface::DeferMessage(const FDeferredAndroidMessage& DeferredMessage)
{
	FScopeLock Lock(&TouchInputCriticalSection);
	// Get the index we should be writing to
	int32 Index = DeferredMessageQueueLastEntryIndex++;

	if (Index >= MAX_DEFERRED_MESSAGE_QUEUE_SIZE)
	{
		// Actually, if the queue is full, drop the message and increment a counter of drops
		DeferredMessageQueueDroppedCount++;
		return;
	}
	DeferredMessages[Index] = DeferredMessage;
}
