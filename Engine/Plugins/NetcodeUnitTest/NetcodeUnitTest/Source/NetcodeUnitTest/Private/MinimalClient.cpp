// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MinimalClient.h"

#include "Misc/FeedbackContext.h"
#include "Misc/NetworkVersion.h"
#include "Engine/Engine.h"
#include "Engine/ActorChannel.h"
#include "Net/DataChannel.h"
#include "OnlineSubsystemTypes.h"

#include "NetcodeUnitTest.h"
#include "UnitTestEnvironment.h"
#include "UnitTestManager.h"
#include "Net/NUTUtilNet.h"
#include "Net/UnitTestNetDriver.h"
#include "Net/UnitTestNetConnection.h"
#include "Net/UnitTestChannel.h"


// @todo #JohnB: Eventually, you want to remove this classes dependence on the unit test code, and have it be independent.
//					For now though, just allow the dependence, to ease the transition


/**
 * UMinimalClient
 */

UMinimalClient::UMinimalClient(const FObjectInitializer& ObjectInitializor)
	: Super(ObjectInitializor)
	, FProtMinClientParms()
	, FProtMinClientHooks()
	, bConnected(false)
	, UnitWorld(nullptr)
	, UnitNetDriver(nullptr)
	, UnitConn(nullptr)
	, ControlBunchSequence(0)
	, PendingNetActorChans()
{
}

bool UMinimalClient::Connect(FMinClientParms Parms, FMinClientHooks Hooks)
{
	bool bSuccess = false;

	// Make sure we're not already setup
	check(Hooks.ConnectedDel.IsBound());
	check(MinClientFlags == EMinClientFlags::None && !ConnectedDel.IsBound());

	// @todo #JohnB: Add validation of minimal client flags, through another function, here.

	Parms.CopyParms(*this);
	Hooks.CopyHooks(*this);

	// @todo #JohnB: Remove once fully migrated to minimal client
	Owner->ControlBunchSequence = &ControlBunchSequence;

	if (UnitWorld == nullptr)
	{
		// Make all of this happen in a blank, newly constructed world
		UnitWorld = NUTNet::CreateUnitTestWorld();

		if (UnitWorld != nullptr)
		{
			if (ConnectMinimalClient())
			{
				bSuccess = true;

				if (GEngine != nullptr)
				{
#if TARGET_UE4_CL >= CL_DEPRECATEDEL
					InternalNotifyNetworkFailureDelegateHandle = GEngine->OnNetworkFailure().AddUObject(this,
																	&UMinimalClient::InternalNotifyNetworkFailure);
#else
					GEngine->OnNetworkFailure().AddUObject(this, &UMinimalClient::InternalNotifyNetworkFailure);
#endif
				}


				// @todo #JohnB: Deprecate this
#if 1
				Owner->UnitNetDriver = UnitNetDriver;
				Owner->UnitConn = UnitConn;
#endif
			}
			else
			{
				FString LogMsg = TEXT("Failed to create minimal client connection");

				UNIT_LOG_OBJ(Owner, ELogType::StatusFailure, TEXT("%s"), *LogMsg);
				UNIT_STATUS_LOG_OBJ(Owner, ELogType::StatusVerbose, TEXT("%s"), *LogMsg);
			}
		}
		else
		{
			FString LogMsg = TEXT("Failed to create unit test world");

			UNIT_LOG_OBJ(Owner, ELogType::StatusFailure, TEXT("%s"), *LogMsg);
			UNIT_STATUS_LOG_OBJ(Owner, ELogType::StatusVerbose, TEXT("%s"), *LogMsg);
		}
	}
	else
	{
		FString LogMsg = TEXT("Unit test world already exists, can't create minimal client");

		UNIT_LOG_OBJ(Owner, ELogType::StatusWarning, TEXT("%s"), *LogMsg);
		UNIT_STATUS_LOG_OBJ(Owner, ELogType::StatusVerbose, TEXT("%s"), *LogMsg);
	}

	return bSuccess;
}

void UMinimalClient::Cleanup()
{
	if (UnitConn != nullptr)
	{
		UnitConn->Close();
	}

	DisconnectMinimalClient();

	if (UnitNetDriver != nullptr)
	{
		UnitNetDriver->Notify = nullptr;
	}

	UnitNetDriver = nullptr;

	UnitConn = nullptr;

	// @todo #JohnB: Deprecate this
#if 1
	Owner->UnitNetDriver = nullptr;
	Owner->UnitConn = nullptr;
#endif

	ControlBunchSequence = 0;

	// @todo #JohnB: Remove once fully migrated to minimal client
	Owner->ControlBunchSequence = nullptr;

	PendingNetActorChans.Empty();

	if (GEngine != nullptr)
	{
#if TARGET_UE4_CL >= CL_DEPRECATEDEL
		GEngine->OnNetworkFailure().Remove(InternalNotifyNetworkFailureDelegateHandle);
#else
		GEngine->OnNetworkFailure().RemoveUObject(this, &UMinimalClient::InternalNotifyNetworkFailure);
#endif
	}

	// Immediately cleanup (or rather, at start of next tick, as that's earliest possible time) after sending the RPC
	if (UnitWorld != nullptr)
	{
		NUTNet::MarkUnitTestWorldForCleanup(UnitWorld);
		UnitWorld = nullptr;
	}
}

void UMinimalClient::UnitTick(float DeltaTimm)
{
	if (!!(MinClientFlags & EMinClientFlags::NotifyNetActors) && PendingNetActorChans.Num() > 0 && UnitConn != nullptr)
	{
		for (int32 i=PendingNetActorChans.Num()-1; i>=0; i--)
		{
			UActorChannel* CurChan = Cast<UActorChannel>(UnitConn->Channels[PendingNetActorChans[i]]);

			if (CurChan != nullptr && CurChan->Actor != nullptr)
			{
				NetActorDel.ExecuteIfBound(CurChan, CurChan->Actor);
				PendingNetActorChans.RemoveAt(i);
			}
		}
	}
}

bool UMinimalClient::IsTickable() const
{
	return (!!(MinClientFlags & EMinClientFlags::NotifyNetActors) && PendingNetActorChans.Num() > 0);
}

bool UMinimalClient::ConnectMinimalClient()
{
	bool bSuccess = false;

	check(UnitWorld != nullptr);
	check(UnitNetDriver == nullptr);

	UnitNetDriver = NUTNet::CreateUnitTestNetDriver(UnitWorld);

	if (UnitNetDriver != nullptr)
	{
		UnitNetDriver->InitialConnectTimeout = FMath::Max(UnitNetDriver->InitialConnectTimeout, (float)Owner->UnitTestTimeout);
		UnitNetDriver->ConnectionTimeout = FMath::Max(UnitNetDriver->ConnectionTimeout, (float)Owner->UnitTestTimeout);

		if (!(MinClientFlags & EMinClientFlags::SendRPCs) || !!(MinClientFlags & EMinClientFlags::DumpSendRPC))
		{
			UUnitTestNetDriver* CurUnitDriver = CastChecked<UUnitTestNetDriver>(UnitNetDriver);

			CurUnitDriver->SendRPCDel.BindUObject(this, &UMinimalClient::NotifySendRPC);
		}

		bool bBeaconConnect = !!(MinClientFlags & EMinClientFlags::BeaconConnect);
		FString ConnectAddress = (bBeaconConnect ? BeaconAddress : ServerAddress);
		FURL DefaultURL;
		FURL TravelURL(&DefaultURL, *ConnectAddress, TRAVEL_Absolute);
		FString ConnectionError;
		bSuccess = UnitNetDriver->InitConnect(this, TravelURL, ConnectionError);

		if (bSuccess)
		{
			UnitConn = UnitNetDriver->ServerConnection;

			FString LogMsg = FString::Printf(TEXT("Successfully created minimal client connection to IP '%s'"), *ConnectAddress);

			UNIT_LOG_OBJ(Owner, ELogType::StatusImportant, TEXT("%s"), *LogMsg);
			UNIT_STATUS_LOG_OBJ(Owner, ELogType::StatusVerbose, TEXT("%s"), *LogMsg);


			UUnitTestNetConnection* CurUnitConn = CastChecked<UUnitTestNetConnection>(UnitConn);
			UUnitTestChannel* UnitControlChan = CastChecked<UUnitTestChannel>(UnitConn->Channels[0]);

			CurUnitConn->MinClient = this;
			CurUnitConn->SocketHook.MinClient = this;
			UnitControlChan->MinClient = this;

#if TARGET_UE4_CL >= CL_STATELESSCONNECT
			if (UnitConn->Handler.IsValid())
			{
				UnitConn->Handler->BeginHandshaking(
										FPacketHandlerHandshakeComplete::CreateUObject(this, &UMinimalClient::SendInitialJoin));
			}
			else
			{
				SendInitialJoin();
			}
#endif
		}
		else
		{
			UE_LOG(LogUnitTest, Error, TEXT("Failed to kickoff connect to IP '%s', error: %s"), *ConnectAddress, *ConnectionError);
		}
	}
	else
	{
		UE_LOG(LogUnitTest, Error, TEXT("Failed to create an instance of the unit test net driver"));
	}

	return bSuccess;
}

void UMinimalClient::SendInitialJoin()
{
	// @todo #JohnB: Deprecate ControlBunchSequence as soon as you can - and switch to using OutReliable on its own
	ControlBunchSequence = UnitConn->OutReliable[0];

	FOutBunch* ControlChanBunch = NUTNet::CreateChannelBunch(ControlBunchSequence, UnitConn, CHTYPE_Control, 0);

	if (ControlChanBunch != nullptr)
	{
		// Need to send 'NMT_Hello' to start off the connection (the challenge is not replied to)
		uint8 IsLittleEndian = uint8(PLATFORM_LITTLE_ENDIAN);

		// We need to construct the NMT_Hello packet manually, for the initial connection
		uint8 MessageType = NMT_Hello;

		*ControlChanBunch << MessageType;
		*ControlChanBunch << IsLittleEndian;

		uint32 LocalNetworkVersion = FNetworkVersion::GetLocalNetworkVersion();
		*ControlChanBunch << LocalNetworkVersion;


		bool bSkipControlJoin = !!(MinClientFlags & EMinClientFlags::SkipControlJoin);
		bool bBeaconConnect = !!(MinClientFlags & EMinClientFlags::BeaconConnect);
		TSharedPtr<const FUniqueNetId> DudPtr = MakeShareable(new FUniqueNetIdString(TEXT("Dud")));
		FUniqueNetIdRepl PlayerUID(DudPtr);
		FString BlankStr = TEXT("");
		FString ConnectURL = UUnitTest::UnitEnv->GetDefaultClientConnectURL();

		if (JoinUID != nullptr)
		{
			PlayerUID = *JoinUID;
		}

		if (bBeaconConnect)
		{
			if (!bSkipControlJoin)
			{
				MessageType = NMT_BeaconJoin;
				*ControlChanBunch << MessageType;
				*ControlChanBunch << BeaconType;
				*ControlChanBunch << PlayerUID;

				// Also immediately ack the beacon GUID setup; we're just going to let the server setup the client beacon,
				// through the actor channel
				MessageType = NMT_BeaconNetGUIDAck;
				*ControlChanBunch << MessageType;
				*ControlChanBunch << BeaconType;
			}
		}
		else
		{
			// Then send NMT_Login
			MessageType = NMT_Login;
			*ControlChanBunch << MessageType;
			*ControlChanBunch << BlankStr;
			*ControlChanBunch << ConnectURL;
			*ControlChanBunch << PlayerUID;


			// Now send NMT_Join, to trigger a fake player, which should then trigger replication of basic actor channels
			if (!bSkipControlJoin)
			{
				MessageType = NMT_Join;
				*ControlChanBunch << MessageType;
			}
		}


		// Big hack: Store OutRec value on the unit test control channel, to enable 'retry-send' code
		if (UnitConn->Channels[0] != nullptr)
		{
			UnitConn->Channels[0]->OutRec = ControlChanBunch;
		}

		UnitConn->SendRawBunch(*ControlChanBunch, true);


		// At this point, fire of notification that we are connected
		bConnected = true;

		ConnectedDel.ExecuteIfBound();
	}
	else
	{
		UE_LOG(LogUnitTest, Error, TEXT("Failed to kickoff connection, could not create control channel bunch."));
	}
}

void UMinimalClient::DisconnectMinimalClient()
{
	if (UnitWorld != nullptr && UnitNetDriver != nullptr)
	{
		GEngine->DestroyNamedNetDriver(UnitWorld, UnitNetDriver->NetDriverName);
	}
}

bool UMinimalClient::NotifySendRPC(AActor* Actor, UFunction* Function, void* Parameters, FOutParmRec* OutParms, FFrame* Stack,
									UObject* SubObject)
{
	bool bAllowRPC = !!(MinClientFlags & EMinClientFlags::SendRPCs);

	// Pass on to the delegate, and give it an opportunity to override whether the RPC is sent
	SendRPCDel.ExecuteIfBound(bAllowRPC, Actor, Function, Parameters, OutParms, Stack, SubObject);

	if (bAllowRPC)
	{
		if (!!(MinClientFlags & EMinClientFlags::DumpSendRPC))
		{
			UNIT_LOG_OBJ(Owner, ELogType::StatusDebug, TEXT("Send RPC '%s' for actor '%s' (SubObject '%s')"), *Function->GetName(),
						*Actor->GetFullName(), (SubObject != NULL ? *SubObject->GetFullName() : TEXT("nullptr")));
		}
	}
	else
	{
		if (!(MinClientFlags & EMinClientFlags::SendRPCs))
		{
			UNIT_LOG_OBJ(Owner, , TEXT("Blocking send RPC '%s' in actor '%s' (SubObject '%s')"), *Function->GetName(),
						*Actor->GetFullName(), (SubObject != NULL ? *SubObject->GetFullName() : TEXT("nullptr")));
		}
	}

	return bAllowRPC;
}

void UMinimalClient::InternalNotifyNetworkFailure(UWorld* InWorld, UNetDriver* InNetDriver, ENetworkFailure::Type FailureType,
													const FString& ErrorString)
{
	if (InNetDriver == (UNetDriver*)UnitNetDriver)
	{
		UNIT_EVENT_BEGIN(Owner);

		NetworkFailureDel.ExecuteIfBound(FailureType, ErrorString);

		UNIT_EVENT_END;
	}
}

bool UMinimalClient::NotifyAcceptingChannel(UChannel* Channel)
{
	bool bAccepted = false;

	if (Channel->ChType == CHTYPE_Actor)
	{
		bAccepted = !!(MinClientFlags & EMinClientFlags::AcceptActors);

		if (!!(MinClientFlags & EMinClientFlags::NotifyNetActors))
		{
			PendingNetActorChans.Add(Channel->ChIndex);
		}
	}

	return bAccepted;
}
