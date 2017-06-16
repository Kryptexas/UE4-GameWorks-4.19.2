// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Net/UnitTestNetConnection.h"

#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"

#include "MinimalClient.h"
#include "NUTUtil.h"
#include "NUTUtilDebug.h"

#include "Net/UnitTestPackageMap.h"
#include "Net/UnitTestActorChannel.h"


/**
 * Static variables
 */

bool UUnitTestNetConnection::bForceEnableHandler = false;


/**
 * Default constructor
 */
UUnitTestNetConnection::UUnitTestNetConnection(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, MinClient(nullptr)
	, NotifyHandleClientPlayerDel()
	, SocketHook()
	, bDisableValidateSend(false)
{
	// Work around a minor UNetConnection bug, where QueuedBits is not initialized, until after the first Tick
	QueuedBits = -(MAX_PACKET_SIZE * 8);
}

void UUnitTestNetConnection::InitBase(UNetDriver* InDriver, class FSocket* InSocket, const FURL& InURL, EConnectionState InState,
										int32 InMaxPacket/*=0*/, int32 InPacketOverhead/*=0*/)
{
	Super::InitBase(InDriver, InSocket, InURL, InState, InMaxPacket, InPacketOverhead);

	// Replace the package map
#if TARGET_UE4_CL < CL_DEPRECATENEW
	PackageMap = new(this) UUnitTestPackageMap(FObjectInitializer(), this, Driver->GuidCache);
#else
	PackageMap = NewObject<UUnitTestPackageMap>(this);
	Cast<UUnitTestPackageMap>(PackageMap)->Initialize(this, Driver->GuidCache);
#endif
}

#if TARGET_UE4_CL < CL_INITCONNPARAM
void UUnitTestNetConnection::InitConnection(UNetDriver* InDriver, EConnectionState InState, const FURL& InURL,
											int32 InConnectionSpeed/*=0*/)
#else
void UUnitTestNetConnection::InitConnection(UNetDriver* InDriver, EConnectionState InState, const FURL& InURL,
											int32 InConnectionSpeed/*=0*/, int32 InMaxPacket/*=0*/)
#endif
{
	Super::InitConnection(InDriver, InState, InURL, InConnectionSpeed);

	// Replace the package map
#if TARGET_UE4_CL < CL_DEPRECATENEW
	PackageMap = new(this) UUnitTestPackageMap(FObjectInitializer(), this, Driver->GuidCache);
#else
	PackageMap = NewObject<UUnitTestPackageMap>(this);
	Cast<UUnitTestPackageMap>(PackageMap)->Initialize(this, Driver->GuidCache);
#endif
}

void UUnitTestNetConnection::InitHandler()
{
	// @todo #JohnB: Broken/redundant, after addition of stateless challenge?
#if 0
	// Copy-pasted from UNetConnection
	if (bForceEnableHandler)
	{
		check(!Handler.IsValid());

		Handler = MakeUnique<PacketHandler>();

		Handler->bEnabled = true;

		if (Handler.IsValid())
		{
			Handler::Mode Mode = Driver->ServerConnection != nullptr ? Handler::Mode::Client : Handler::Mode::Server;
			Handler->Initialize(Mode);

			MaxPacketHandlerBits = Handler->GetTotalPacketOverheadBits();
		}
	}
	else
#endif
	{
		Super::InitHandler();
	}
}

void UUnitTestNetConnection::LowLevelSend(void* Data, int32 CountBytes, int32 CountBits)
{
	bool bBlockSend = false;

	if (MinClient != nullptr)
	{
		UClientUnitTest* Owner = MinClient->GetOwner();

		MinClient->LowLevelSendDel.ExecuteIfBound(Data, CountBytes, bBlockSend);

		if (!!(MinClient->GetMinClientFlags() & EMinClientFlags::DumpSendRaw))
		{
			UNIT_LOG_OBJ(Owner, ELogType::StatusDebug, TEXT("NotifySendRawPacket: Packet dump:"));

			UNIT_LOG_BEGIN(Owner, ELogType::StatusDebug | ELogType::StyleMonospace);
			NUTDebug::LogHexDump((const uint8*)Data, CountBytes, true, true);
			UNIT_LOG_END();
		}
	}

	if (!bBlockSend)
	{
		// Only attach the SocketHook when it is needed - not all the time
		SocketHook.Attach(Socket);

		Super::LowLevelSend(Data, CountBytes, CountBits);

		SocketHook.Detach(Socket);

		GSentBunch = !SocketHook.bLastSendToBlocked;
	}
}

void UUnitTestNetConnection::ValidateSendBuffer()
{
	if (!bDisableValidateSend)
	{
		Super::ValidateSendBuffer();
	}
}

void UUnitTestNetConnection::ReceivedRawPacket(void* Data, int32 Count)
{
	GActiveReceiveUnitConnection = this;

	if (MinClient != nullptr)
	{
		UClientUnitTest* Owner = MinClient->GetOwner();

		MinClient->ReceivedRawPacketDel.ExecuteIfBound(Data, Count);

		if (Owner != nullptr)
		{
			if (!!(MinClient->GetMinClientFlags() & EMinClientFlags::DumpReceivedRaw))
			{
				UNIT_LOG_OBJ(Owner, ELogType::StatusDebug, TEXT("NotifyReceivedRawPacket: Packet dump:"));

				UNIT_LOG_BEGIN(Owner, ELogType::StatusDebug | ELogType::StyleMonospace);
				NUTDebug::LogHexDump((const uint8*)Data, Count, true, true);
				UNIT_LOG_END();
			}
		}
	}


	// Selectively override the actor channel, for unit test net connections (client only creates actor channels, within this call)
	UClass* OrigClass = UChannel::ChannelClasses[CHTYPE_Actor];
	UChannel::ChannelClasses[CHTYPE_Actor] = UUnitTestActorChannel::StaticClass();

	Super::ReceivedRawPacket(Data, Count);

	UChannel::ChannelClasses[CHTYPE_Actor] = OrigClass;


	GActiveReceiveUnitConnection = nullptr;
}

void UUnitTestNetConnection::HandleClientPlayer(class APlayerController* PC, class UNetConnection* NetConnection)
{
	// Implement only essential parts of the original function, as we want to block most of it (triggers level change code)
	PC->Role = ROLE_AutonomousProxy;
	PC->NetConnection = NetConnection;

	PlayerController = PC;
	OwningActor = PC;

	// @todo #JohnBReview: This might cause undesirable behaviour, if - for example - HandleDisconnect gets called by
	//				RPC's, so may want to create a fake localplayer instead
	PC->Player = GEngine->GetFirstGamePlayer(NUTUtil::GetPrimaryWorld());

	// Sometimes, e.g. when executing in a commandlet, there is no available player, and one has to be created
	if (PC->Player == nullptr)
	{
		// Do nothing, other than just create the raw object
		PC->Player = NewObject<ULocalPlayer>(GEngine, GEngine->LocalPlayerClass);
	}


	// Pass on notification
	NotifyHandleClientPlayerDel.ExecuteIfBound(PC, NetConnection);
}


