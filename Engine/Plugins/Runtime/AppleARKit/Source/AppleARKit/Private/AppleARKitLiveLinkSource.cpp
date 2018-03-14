// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AppleARKitLiveLinkSource.h"
#include "Package.h"
#include "Misc/ConfigCacheIni.h"
#include "UObjectGlobals.h"
#include "ObjectMacros.h"
#include "ARSystem.h"
#include "ARBlueprintLibrary.h"
#include "AppleARKitModule.h"
#include "Features/IModularFeatures.h"

#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "Sockets.h"

#include "AppleARKitSettings.h"
#include "ARTrackable.h"


TSharedPtr<ILiveLinkSourceARKit> FAppleARKitLiveLinkSourceFactory::CreateLiveLinkSource(bool bCreateRemotePublisher)
{
	IModularFeatures& ModularFeatures = IModularFeatures::Get();

	if (ModularFeatures.IsModularFeatureAvailable(ILiveLinkClient::ModularFeatureName))
	{
		ILiveLinkClient* LiveLinkClient = &IModularFeatures::Get().GetModularFeature<ILiveLinkClient>(ILiveLinkClient::ModularFeatureName);
		TSharedPtr <ILiveLinkSourceARKit> Source = MakeShareable(new FAppleARKitLiveLinkSource(bCreateRemotePublisher));
		LiveLinkClient->AddSource(Source);
		return Source;
	}
	return TSharedPtr<ILiveLinkSourceARKit>();
}

void FAppleARKitLiveLinkSourceFactory::CreateLiveLinkRemoteListener()
{
	static FAppleARKitLiveLinkRemoteListener* Listener = nullptr;
	if (Listener == nullptr)
	{
		Listener = new FAppleARKitLiveLinkRemoteListener();
		if (!Listener->InitReceiveSocket())
		{
			delete Listener;
			Listener = nullptr;
		}
	}
}

FAppleARKitLiveLinkSource::FAppleARKitLiveLinkSource(bool bCreateRemotePublisher) :
	Client(nullptr)
	, LastFramePublished(0)
{
#if PLATFORM_IOS
	if (bCreateRemotePublisher)
	{
		// Only send from iOS to desktop
		// This will perform the sending of the data to the remote
		FAppleARKitLiveLinkRemotePublisher* Publisher = new FAppleARKitLiveLinkRemotePublisher();
		if (Publisher->InitSendSocket())
		{
			RemoteLiveLinkPublisher = MakeShareable(Publisher);
		}
		else
		{
			UE_LOG(LogAppleARKit, Warning, TEXT("Failed to create LiveLink remote publisher, so no data will be sent out"));
			delete Publisher;
		}
	}
#endif
}

void FAppleARKitLiveLinkSource::ReceiveClient(ILiveLinkClient* InClient, FGuid InSourceGuid)
{
	Client = InClient;
	SourceGuid = InSourceGuid;
}

bool FAppleARKitLiveLinkSource::IsSourceStillValid()
{
	return Client != nullptr;
}

bool FAppleARKitLiveLinkSource::RequestSourceShutdown()
{
	Client = nullptr;
	return true;
}

FText FAppleARKitLiveLinkSource::GetSourceMachineName() const
{
	return FText().FromString(FPlatformProcess::ComputerName());
}

FText FAppleARKitLiveLinkSource::GetSourceStatus() const
{
	return NSLOCTEXT( "AppleARKitLiveLink", "AppleARKitLiveLinkStatus", "Active" );
}

FText FAppleARKitLiveLinkSource::GetSourceType() const
{
	return NSLOCTEXT( "AppleARKitLiveLink", "AppleARKitLiveLinkSourceType", "Apple AR Face Tracking" );
}

static FName ParseEnumName(FName EnumName)
{
	const int32 BlendShapeEnumNameLength = 19;
	FString EnumString = EnumName.ToString();
	return FName(*EnumString.Right(EnumString.Len() - BlendShapeEnumNameLength));
}

void FAppleARKitLiveLinkSource::PublishBlendShapes(FName SubjectName, double Timestamp, uint32 FrameNumber, const FARBlendShapeMap& FaceBlendShapes)
{
	check(Client != nullptr);

	if (SubjectName != LastSubjectName)
	{
		Client->ClearSubject(LastSubjectName);
		// We need to publish a skeleton for this subject name even though we doesn't use one
		Client->PushSubjectSkeleton(SourceGuid, SubjectName, FLiveLinkRefSkeleton());
	}
	LastSubjectName = SubjectName;

	const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("EARFaceBlendShape"), true);
	if (EnumPtr != nullptr)
	{
		static FLiveLinkFrameData LiveLinkFrame;

		LiveLinkFrame.WorldTime = Timestamp;

		TArray<FLiveLinkCurveElement>& BlendShapes = LiveLinkFrame.CurveElements;
		
		BlendShapes.Reset((int32)EARFaceBlendShape::MAX);

		// Iterate through all of the blend shapes copying them into the LiveLink data type
		for (int32 Shape = 0; Shape < (int32)EARFaceBlendShape::MAX; Shape++)
		{
			if (FaceBlendShapes.Contains((EARFaceBlendShape)Shape))
			{
				int32 Index = BlendShapes.AddUninitialized(1);
				BlendShapes[Index].CurveName = ParseEnumName(EnumPtr->GetNameByValue(Shape));
				const float CurveValue = FaceBlendShapes.FindChecked((EARFaceBlendShape)Shape);
				BlendShapes[Index].CurveValue = CurveValue;
			}
		}

		// Share the data locally with the LiveLink client
		Client->PushSubjectData(SourceGuid, SubjectName, LiveLinkFrame);
		// Send it to the remote editor via the message bus
		if (RemoteLiveLinkPublisher.IsValid())
		{
			RemoteLiveLinkPublisher->PublishBlendShapes(SubjectName, Timestamp, FrameNumber, FaceBlendShapes);
		}
	}
}

const uint8 BLEND_SHAPE_PACKET_VER = 1;

const uint32 MAX_BLEND_SHAPE_PACKET_SIZE = sizeof(BLEND_SHAPE_PACKET_VER) + sizeof(double) + sizeof(uint32) + sizeof(uint8) + (sizeof(float) * (uint64)EARFaceBlendShape::MAX) + (sizeof(TCHAR) * 256);
const uint32 MIN_BLEND_SHAPE_PACKET_SIZE = sizeof(BLEND_SHAPE_PACKET_VER) + sizeof(double) + sizeof(uint32) + sizeof(uint8) + (sizeof(float) * (uint64)EARFaceBlendShape::MAX) + sizeof(TCHAR);

FAppleARKitLiveLinkRemotePublisher::FAppleARKitLiveLinkRemotePublisher() :
	SendSocket(nullptr),
	SendBuffer(MAX_BLEND_SHAPE_PACKET_SIZE)
{
}

FAppleARKitLiveLinkRemotePublisher::~FAppleARKitLiveLinkRemotePublisher()
{
	if (SendSocket != nullptr)
	{
		SendSocket->Close();
		ISocketSubsystem* SocketSub = ISocketSubsystem::Get();
		SocketSub->DestroySocket(SendSocket);
	}
}

bool FAppleARKitLiveLinkRemotePublisher::InitSendSocket()
{
	TSharedRef<FInternetAddr> Addr = GetSendAddress();
	if (Addr->IsValid())
	{
		ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get();
		// Allocate our socket for sending
		SendSocket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("FAppleARKitLiveLinkRemotePublisher socket"), true);
		SendSocket->SetReuseAddr();
		SendSocket->SetNonBlocking();
	}
	return SendSocket != nullptr;
}

TSharedRef<FInternetAddr> FAppleARKitLiveLinkRemotePublisher::GetSendAddress()
{
	ISocketSubsystem* SocketSub = ISocketSubsystem::Get();
	static TSharedRef<FInternetAddr> SendAddr = SocketSub->CreateInternetAddr();
	if (!SendAddr->IsValid())
	{
		FString RemoteIp;
		if (FParse::Value(FCommandLine::Get(), TEXT("LiveLinkRemoteIp="), RemoteIp))
		{
			int32 LiveLinkPort = GetDefault<UAppleARKitSettings>()->LiveLinkPublishingPort;
			SendAddr->SetPort(LiveLinkPort);
			bool bIsValid = false;
			SendAddr->SetIp(*RemoteIp, bIsValid);
			UE_LOG(LogAppleARKit, Log, TEXT("Sending LiveLink face AR data to address (%s)"), *SendAddr->ToString(true));
		}
	}
	return SendAddr;
}

void FAppleARKitLiveLinkRemotePublisher::PublishBlendShapes(FName SubjectName, double Timestamp, uint32 FrameNumber, const FARBlendShapeMap& FaceBlendShapes)
{
	if (SendSocket != nullptr)
	{
		// Build the packet and send it
		SendBuffer.Reset();
		SendBuffer << BLEND_SHAPE_PACKET_VER;
		SendBuffer << SubjectName;
		SendBuffer << Timestamp;
		SendBuffer << FrameNumber;
		uint8 BlendShapeCount = (uint8)EARFaceBlendShape::MAX;
		check(FaceBlendShapes.Num() == BlendShapeCount);
		SendBuffer << BlendShapeCount;
		// Loop through and send each float for each enum
		for (uint8 BlendShapeIndex = 0; BlendShapeIndex < BlendShapeCount; BlendShapeIndex++)
		{
			SendBuffer << FaceBlendShapes.FindChecked((EARFaceBlendShape)BlendShapeIndex);
		}

		// Now send the packet
		uint32 SourceBufferSize = SendBuffer.GetByteCount();
		check(SourceBufferSize <= MAX_BLEND_SHAPE_PACKET_SIZE && "Max packet size for face blends was exceeded");
		int32 AmountSent = 0;
		if (!SendSocket->SendTo(SendBuffer, SourceBufferSize, AmountSent, *GetSendAddress()) ||
			AmountSent != SourceBufferSize)
		{
			ISocketSubsystem* SocketSub = ISocketSubsystem::Get();
			UE_LOG(LogAppleARKit, Verbose, TEXT("Failed to send face AR packet with error (%s). Packet size (%d), sent (%d)"), SocketSub->GetSocketError(), SourceBufferSize, AmountSent);
		}
	}
}

FAppleARKitLiveLinkRemoteListener::FAppleARKitLiveLinkRemoteListener() :
	RecvSocket(nullptr)
{
	RecvBuffer.AddUninitialized(MAX_BLEND_SHAPE_PACKET_SIZE);
}

FAppleARKitLiveLinkRemoteListener::~FAppleARKitLiveLinkRemoteListener()
{
	if (RecvSocket != nullptr)
	{
		RecvSocket->Close();
		ISocketSubsystem* SocketSub = ISocketSubsystem::Get();
		SocketSub->DestroySocket(RecvSocket);
	}
}

bool FAppleARKitLiveLinkRemoteListener::InitReceiveSocket()
{
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get();
	TSharedRef<FInternetAddr> Addr = SocketSubsystem->GetLocalBindAddr(*GLog);
	int32 LiveLinkPort = 0;
	// Have to read this value manually since it happens before UObjects are alive
	GConfig->GetInt(TEXT("/Script/AppleARKit.AppleARKitSettings"), TEXT("LiveLinkPublishingPort"), LiveLinkPort, GEngineIni);
	Addr->SetPort(LiveLinkPort);

	RecvSocket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("FAppleARKitLiveLinkRemoteListener socket"));
	if (RecvSocket != nullptr)
	{
		RecvSocket->SetReuseAddr();
		RecvSocket->SetNonBlocking();
		RecvSocket->SetRecvErr();
		// Bind to our listen port
		if (!RecvSocket->Bind(*Addr))
		{
			SocketSubsystem->DestroySocket(RecvSocket);
			RecvSocket = nullptr;
			UE_LOG(LogAppleARKit, Warning, TEXT("Failed to bind to the listen port (%s) for LiveLink face AR receiving with error (%s)"),
				*Addr->ToString(true), SocketSubsystem->GetSocketError());
		}
	}
	return RecvSocket != nullptr;
}

void FAppleARKitLiveLinkRemoteListener::InitLiveLinkSource()
{
	if (!Source.IsValid())
	{
		Source = FAppleARKitLiveLinkSourceFactory::CreateLiveLinkSource(false);
	}
}

void FAppleARKitLiveLinkRemoteListener::Tick(float DeltaTime)
{
	uint32 BytesPending = 0;
	while (RecvSocket->HasPendingData(BytesPending))
	{
		ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get();
		TSharedRef<FInternetAddr> Sender = SocketSubsystem->CreateInternetAddr();

		int32 BytesRead = 0;
		if (RecvSocket->RecvFrom(RecvBuffer.GetData(), RecvBuffer.Num(), BytesRead, *Sender) &&
			// Make sure the packet is a complete one and ignore if it is not
			BytesRead > MIN_BLEND_SHAPE_PACKET_SIZE)
		{
			uint8 PacketVer = 0;
			FName SubjectName;
			double Timestamp = -1.0;
			uint32 FrameNumber = 0;
			uint8 BlendShapeCount = (uint8)EARFaceBlendShape::MAX;

			FNboSerializeFromBuffer FromBuffer(RecvBuffer.GetData(), BytesRead);

			FromBuffer >> PacketVer;
			if (FromBuffer.HasOverflow() || PacketVer != BLEND_SHAPE_PACKET_VER)
			{
				UE_LOG(LogAppleARKit, Verbose, TEXT("Packet overflow reading the packet version for the face AR packet"));
				return;
			}
			FromBuffer >> SubjectName;
			FromBuffer >> Timestamp;
			FromBuffer >> FrameNumber;
			FromBuffer >> BlendShapeCount;
			if (FromBuffer.HasOverflow() || BlendShapeCount != (uint8)EARFaceBlendShape::MAX)
			{
				UE_LOG(LogAppleARKit, Verbose, TEXT("Packet overflow reading the face AR packet's non-array fields"));
				return;
			}

			// Loop through and parse each float for each enum
			for (uint8 BlendShapeIndex = 0; BlendShapeIndex < BlendShapeCount && !FromBuffer.HasOverflow(); BlendShapeIndex++)
			{
				float Value = 0.f;
				FromBuffer >> Value;
				BlendShapes.Add((EARFaceBlendShape)BlendShapeIndex, Value);
			}
			// All of the data was valid, so publish it
			if (!FromBuffer.HasOverflow())
			{
				InitLiveLinkSource();
				if (Source.IsValid())
				{
					Source->PublishBlendShapes(SubjectName, Timestamp, FrameNumber, BlendShapes);
				}
			}
			else
			{
				UE_LOG(LogAppleARKit, Verbose, TEXT("Packet overflow reading the face AR packet's array of blend shapes"));
			}
		}
	}
}
