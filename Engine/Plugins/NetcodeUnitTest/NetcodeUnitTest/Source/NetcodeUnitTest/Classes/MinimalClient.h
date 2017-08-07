// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Engine/PendingNetGame.h"

#include "ClientUnitTest.h"

#include "MinimalClient.generated.h"


/**
 * Flags for configuring the minimal client, what parts of the netcode should be enabled etc.
 */
UENUM()
enum class EMinClientFlags : uint32
{
	None					= 0x00000000,	// No flags

	/** Minimal-client netcode functionality */
	AcceptActors			= 0x00000004,	// Whether or not to accept actor channels (acts as whitelist-only with NotifyAllowNetActor)
	AcceptRPCs				= 0x00000010,	// Whether or not to accept execution of any actor RPC's (they are all blocked by default)
	SendRPCs				= 0x00000020,	// Whether or not to allow RPC sending (NOT blocked by default, @todo JohnB: Add send hook)
	SkipControlJoin			= 0x00000040,	// Whether or not to skip sending NMT_Join upon connect (or NMT_BeaconJoin for beacons)
	BeaconConnect			= 0x00000080,	// Whether or not to connect to the servers beacon (greatly limits the connection)

	/** Unit test events */
	NotifyNetActors			= 0x00400000,	// Whether or not to trigger a 'NotifyNetActor' event, AFTER creation of actor channel actor
	// @todo #JohnB: You want to avoid duplicate ProcessEvent hooks, due to performance issues
	NotifyProcessEvent		= 0x00800000,	// Whether or not to trigger 'NotifyScriptProcessEvent' for every executed local function

	/** Debugging */
	DumpReceivedRaw			= 0x04000000,	// Whether or not to also hex-dump the raw packet receives to the log/log-window
	DumpSendRaw				= 0x08000000,	// Whether or not to also hex-dump the raw packet sends to the log/log-window
	DumpSendRPC				= 0x40000000	// Whether or not to dump RPC sends
};

// Required for bitwise operations with the above enum
ENUM_CLASS_FLAGS(EMinClientFlags);

// @todo #JohnB: Make sure that all flags contained above, are also marked in the FromUnitTestFlags function (if appropriate)

/**
 * Converts any EUnitTestFlags values, to their EMinClientFlags equivalent
 */
inline EMinClientFlags FromUnitTestFlags(EUnitTestFlags Flags)
{
	EMinClientFlags ReturnFlags = EMinClientFlags::None;

	#define CHECK_FLAG(x) ReturnFlags |= !!(Flags & EUnitTestFlags::x) ? EMinClientFlags::x : EMinClientFlags::None

	CHECK_FLAG(AcceptActors);
	CHECK_FLAG(AcceptRPCs);
	CHECK_FLAG(SendRPCs);
	CHECK_FLAG(SkipControlJoin);
	CHECK_FLAG(BeaconConnect);
	CHECK_FLAG(NotifyNetActors);
	CHECK_FLAG(NotifyProcessEvent);
	CHECK_FLAG(DumpReceivedRaw);
	CHECK_FLAG(DumpSendRaw);
	CHECK_FLAG(DumpSendRPC);

	#undef CHECK_FLAG

	return ReturnFlags;
}


// Delegates

/**
 * Delegate for marking the minimal client as having connected fully
 */
DECLARE_DELEGATE(FOnMinClientConnected);

/**
 * Delegate for passing back a network connection failure
 *
 * @param FailureType	The reason for the net failure
 * @param ErrorString	More detailed error information
*/
DECLARE_DELEGATE_TwoParams(FOnMinClientNetworkFailure, ENetworkFailure::Type /*FailureType*/, const FString& /*ErrorString*/);

/**
 * Delegate used to control/block sending of RPC's
 *
 * @param bAllowRPC		Whether or not to allow sending of the RPC
 * @param Actor			The actor the RPC will be called in
 * @param Function		The RPC to call
 * @param Parameters	The parameters data blob
 * @param OutParms		Out parameter information (irrelevant for RPC's)
 * @param Stack			The script stack
 * @param SubObject		The sub-object the RPC is being called in (if applicable)
*/
DECLARE_DELEGATE_SevenParams(FOnMinClientSendRPC, bool& /*bAllowRPC*/, AActor* /*Actor*/, UFunction* /*Function*/,
									void* /*Parameters*/, FOutParmRec* /*OutParms*/, FFrame* /*Stack*/, UObject* /*SubObject*/);

/**
 * Delegate for hooking the control channel's 'ReceivedBunch' call
 *
 * @param Bunch		The received bunch
*/
DECLARE_DELEGATE_OneParam(FOnMinClientReceivedControlBunch, FInBunch&/* Bunch*/);


/**
 * Delegate for hooking the net connections 'ReceivedRawPacket'
 * 
 * @param Data		The data received
 * @param Count		The number of bytes received
*/
DECLARE_DELEGATE_TwoParams(FOnMinClientReceivedRawPacket, void* /*Data*/, int32& /*Count*/);

/**
 * Delegate for hooking the net connections 'LowLevelSend'
 *
 * @param Data			The data being sent
 * @param Count			The number of bytes being sent
 * @param bBlockSend	Whether or not to block the send (defaults to false)
*/
DECLARE_DELEGATE_ThreeParams(FOnMinClientLowLevelSend, void* /*Data*/, int32 /*Count*/, bool& /*bBlockSend*/);

/**
 * Delegate for hooking the socket level 'SendTo'
 *
 * @param Data			The data being sent
 * @param Count			The number of bytes being sent
 * @param bBlockSend	Whether or not to block the send (defaults to false)
*/
DECLARE_DELEGATE_ThreeParams(FOnMinClientSocketSend, void* /*Data*/, int32 /*Count*/, bool& /*bBlockSend*/);

/**
 * Delegate for notifying on (and optionally blocking) replicated actor creation
 *
 * @param ActorClass	The class of the actor being replicated
 * @param bActorChannel	Whether or not this actor creation is from an actor channel
 * @return				Whether or not to allow creation of the actor
*/
DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnMinClientRepActorSpawn, UClass* /*ActorClass*/, bool /*bActorChannel*/);

/**
 * Delegate for notifying AFTER an actor channel actor has been created
 *
 * @param ActorChannel	The actor channel associated with the actor
 * @param Actor			The actor that has just been created
*/
DECLARE_DELEGATE_TwoParams(FOnMinClientNetActor, UActorChannel* /*ActorChannel*/, AActor* /*Actor*/);


/**
 * Parameters for configuring the minimal client - also directly inherited by UMinimalClient
 */
struct FMinClientParms
{
	friend class UMinimalClient;


	/** The flags used for configuring the minimal client */
	EMinClientFlags MinClientFlags;

	/** The unit test which owns this minimal client */
	// @todo #JohnB: Eventually remove this interdependency
	UClientUnitTest* Owner;

	/** The address of the launched server */
	FString ServerAddress;

	/** The address of the server beacon (if MinClientFlags are set to connect to a beacon) */
	FString BeaconAddress;

	/** If connecting to a beacon, the beacon type name we are connecting to */
	FString BeaconType;

	/** If overriding the UID used for joining, this specifies it */
	FUniqueNetIdRepl* JoinUID;


	/**
	 * Default constructor
	 */
	FMinClientParms()
		: MinClientFlags(EMinClientFlags::None)
		, Owner(nullptr)
		, ServerAddress()
		, BeaconAddress()
		, BeaconType()
		, JoinUID(nullptr)
	{
	}

protected:
	void CopyParms(FMinClientParms& Target)
	{
		Target.MinClientFlags = MinClientFlags;
		Target.Owner = Owner;
		Target.ServerAddress = ServerAddress;
		Target.BeaconAddress = BeaconAddress;
		Target.BeaconType = BeaconType;
		Target.JoinUID = JoinUID;
	}
};


/**
 * Delegate hooks for the minimal client - also directly inherited by UMinimalClient
 */
struct FMinClientHooks
{
	friend class UMinimalClient;


	/** Delegate for notifying of successful minimal client connection*/
	FOnMinClientConnected ConnectedDel;

	/** Delegate notifying of network failure */
	FOnMinClientNetworkFailure NetworkFailureDel;

	/** Delegate for notifying/controlling RPC sends */
	FOnMinClientSendRPC SendRPCDel;

	/** Delegate for notifying of control channel bunches */
	FOnMinClientReceivedControlBunch ReceivedControlBunchDel;

	/** Delegate for notifying of net connection raw packet receives */
	FOnMinClientReceivedRawPacket ReceivedRawPacketDel;

	/** Delegate for notifying of net connection low level packet sends */
	FOnMinClientLowLevelSend LowLevelSendDel;

	/** Delegate for notifying of socket level packet sends */
	FOnMinClientSocketSend SocketSendDel;

	/** Delegate for notifying/controlling replicated actor spawning */
	FOnMinClientRepActorSpawn RepActorSpawnDel;

	/** Delegate for notifying AFTER net actor creation */
	FOnMinClientNetActor NetActorDel;


	/**
	 * Default constructor
	 */
	FMinClientHooks()
		: ConnectedDel()
		, NetworkFailureDel()
		, SendRPCDel()
		, ReceivedControlBunchDel()
		, ReceivedRawPacketDel()
		, LowLevelSendDel()
		, SocketSendDel()
		, RepActorSpawnDel()
		, NetActorDel()
	{
	}

protected:
	void CopyHooks(FMinClientHooks& Target)
	{
		Target.ConnectedDel = ConnectedDel;
		Target.NetworkFailureDel = NetworkFailureDel;
		Target.SendRPCDel = SendRPCDel;
		Target.ReceivedControlBunchDel = ReceivedControlBunchDel;
		Target.ReceivedRawPacketDel = ReceivedRawPacketDel;
		Target.LowLevelSendDel = LowLevelSendDel;
		Target.SocketSendDel = SocketSendDel;
		Target.RepActorSpawnDel = RepActorSpawnDel;
		Target.NetActorDel = NetActorDel;
	}
};

// Sidestep header parser enforcement of 'public' on inheritance below
struct FProtMinClientParms : protected FMinClientParms {};
struct FProtMinClientHooks : protected FMinClientHooks {};


/**
 * Base class for implementing a barebones/stripped-down game client, capable of connecting to a regular game server,
 * but stripped/locked-down so that the absolute minimum of client/server netcode functionality is executed, for connecting the client.
 */
UCLASS()
class NETCODEUNITTEST_API UMinimalClient : public UObject, public FNetworkNotify, public FProtMinClientParms, public FProtMinClientHooks
{
	friend class UUnitTestNetConnection;
	friend class UUnitTestChannel;
	friend class UUnitTestPackageMap;
	friend class FSocketHook;

	GENERATED_UCLASS_BODY()

public:
	/**
	 * Connects the minimal client to a server, with Parms specifying the server details and minimal client configuration,
	 * and passing back the low level netcode events specified by Hooks.
	 *
	 * @param Parms		The server parameters and minimal client configuration.
	 * @param Hooks		The delegates for hooking low level netcode events in the minimal client.
	 * @return			Whether or not the connection kicked off successfully.
	 */
	virtual bool Connect(FMinClientParms Parms, FMinClientHooks Hooks);

	/**
	 * Disconnects and cleans up the minimal client.
	 */
	virtual void Cleanup();


	// Accessors

	/**
	 * Retrieves the unit test owning the minimal client
	 */
	FORCEINLINE UClientUnitTest* GetOwner()
	{
		return Owner;
	}

	/**
	 * Retrieve the value of MinClientFlags
	 */
	FORCEINLINE EMinClientFlags GetMinClientFlags()
	{
		return MinClientFlags;
	}

	/**
	 * Retrieve the value of UnitWorld
	 */
	FORCEINLINE UWorld* GetUnitWorld()
	{
		return UnitWorld;
	}

	/**
	 * Retrieve the value of UnitConn
	 */
	FORCEINLINE UNetConnection* GetConn()
	{
		return UnitConn;
	}


	/**
	 * Ticks the minimal client, through the owner unit tests UnitTick function
	 */
	virtual void UnitTick(float DeltaTime);

	/**
	 * Whether or not the minimal client requires ticking.
	 */
	bool IsTickable() const;

	/**
	 * Whether or not the minimal client is connected to the server
	 */
	FORCEINLINE bool IsConnected()
	{
		return bConnected;
	}


protected:
	/**
	 * Creates the minimal client state, and connects to the server
	 *
	 * @return	Whether or not the fake player was successfully created
	 */
	bool ConnectMinimalClient();

	/**
	 * Sends the packet for triggering the initial join (usually is delayed, by the PacketHandler)
	 */
	void SendInitialJoin();

	/**
	 * Disconnects the minimal client - including destructing the net driver and such (tears down the whole connection)
	 * NOTE: Based upon the HandleDisconnect function, except removing parts that are undesired (e.g. which trigger level switch)
	 */
	void DisconnectMinimalClient();

	/**
	 * See FOnMinClientSendRPC
	 */
	virtual bool NotifySendRPC(AActor* Actor, UFunction* Function, void* Parameters, FOutParmRec* OutParms, FFrame* Stack,
								UObject* SubObject);


protected:
	void InternalNotifyNetworkFailure(UWorld* InWorld, UNetDriver* InNetDriver, ENetworkFailure::Type FailureType,
										const FString& ErrorString);



	// FNetworkNotify
protected:
	virtual EAcceptConnection::Type NotifyAcceptingConnection() override
	{
		return EAcceptConnection::Ignore;
	}

	virtual void NotifyAcceptedConnection(UNetConnection* Connection) override
	{
	}

	virtual bool NotifyAcceptingChannel(UChannel* Channel) override;

	virtual void NotifyControlMessage(UNetConnection* Connection, uint8 MessageType, FInBunch& Bunch) override
	{
	}


	/** Runtime variables */
protected:
	/** Whether or not the minimal client is connected */
	bool bConnected;

	/** Stores a reference to the created fake world, for execution and later cleanup */
	UWorld* UnitWorld;

	/** Stores a reference to the created unit test net driver, for execution and later cleanup (always a UUnitTestNetDriver) */
	UNetDriver* UnitNetDriver;

	/** Stores a reference to the server connection (always a 'UUnitTestNetConnection') */
	UNetConnection* UnitConn;

	/** For the unit test control channel, this tracks the current bunch sequence */
	int32 ControlBunchSequence;

	/** If notifying of net actor creation, this keeps track of new actor channel indexes pending notification */
	TArray<int32> PendingNetActorChans;


#if TARGET_UE4_CL >= CL_DEPRECATEDEL
private:
	/** Handle to the registered InternalNotifyNetworkFailure delegate */
	FDelegateHandle InternalNotifyNetworkFailureDelegateHandle;
#endif
};
