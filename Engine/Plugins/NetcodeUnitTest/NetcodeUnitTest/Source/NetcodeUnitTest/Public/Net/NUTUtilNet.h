// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Channel.h"
#include "NetcodeUnitTest.h"
#include "Engine/PendingNetGame.h"
#include "Engine/World.h"
#include "Sockets.h"

class APlayerController;
class FInBunch;
class FInternetAddr;
class FOutBunch;
class UMinimalClient;
class UNetConnection;
class UNetDriver;
class UUnitTestChannel;
class UUnitTestNetDriver;
struct FUniqueNetIdRepl;
class UClientUnitTest;


/**
 * A transparent FSocket-hook, for capturing lowest-level socket operations (use Attach/Detach functions for hooking)
 *
 * NOTE: Not compatible with all net subsystems, e.g. some subsystems expect a specific FSocket-subclass, and statically cast as such.
 * NOTE: Best if only temporarily attached, or the original socket may leak (no attempt is made to handle destruction properly).
 */
class FSocketHook : public FSocket
{
public:
	/**
	 * Base constructor
	 */
	FSocketHook()
		: MinClient(nullptr)
		, SendToDel()
		, bLastSendToBlocked(false)
		, HookedSocket(nullptr)
	{
	}

	/**
	 * Base constructor
	 */
	FSocketHook(FSocket* InHookedSocket)
		: FSocket((InHookedSocket != nullptr ? InHookedSocket->GetSocketType() : ESocketType::SOCKTYPE_Datagram),
					(InHookedSocket != nullptr ? InHookedSocket->GetDescription() + TEXT(" (hooked)") : TEXT("(hooked)")))
		, MinClient(nullptr)
		, SendToDel()
		, bLastSendToBlocked(false)
		, HookedSocket(InHookedSocket)
	{
	}

	/**
	 * Attach the hook to the specified FSocket* variable
	 *
	 * @param InSocketVar	The socket var to hook
	 */
	FORCEINLINE void Attach(FSocket*& InSocketVar)
	{
		check(InSocketVar != this);
		check(HookedSocket == nullptr);

		HookedSocket = InSocketVar;
		InSocketVar = this;
	}

	/**
	 * Detach the hook from the specified FSocket* variable
	 *
	 * @param InSocketVar	The socket var to detach from 
	 */
	FORCEINLINE void Detach(FSocket*& InSocketVar)
	{
		check(InSocketVar == this);
		check(HookedSocket != nullptr);

		InSocketVar = HookedSocket;
		HookedSocket = nullptr;
	}

	virtual ~FSocketHook()
	{
	}


	virtual bool Close() override;

	virtual bool Bind(const FInternetAddr& Addr) override;

	virtual bool Connect(const FInternetAddr& Addr) override;

	virtual bool Listen(int32 MaxBacklog) override;

	virtual bool HasPendingConnection(bool& bHasPendingConnection) override;

	virtual bool HasPendingData(uint32& PendingDataSize) override;

	virtual class FSocket* Accept(const FString& InSocketDescription) override;

	virtual class FSocket* Accept(FInternetAddr& OutAddr, const FString& InSocketDescription) override;

	virtual bool SendTo(const uint8* Data, int32 Count, int32& BytesSent, const FInternetAddr& Destination) override;

	virtual bool Send(const uint8* Data, int32 Count, int32& BytesSent) override;

	virtual bool RecvFrom(uint8* Data, int32 BufferSize, int32& BytesRead, FInternetAddr& Source,
							ESocketReceiveFlags::Type Flags) override;

	virtual bool Recv(uint8* Data, int32 BufferSize, int32& BytesRead, ESocketReceiveFlags::Type Flags) override;

	virtual bool Wait(ESocketWaitConditions::Type Condition, FTimespan WaitTime) override;

	virtual ESocketConnectionState GetConnectionState() override;

	virtual void GetAddress(FInternetAddr& OutAddr) override;

	virtual bool GetPeerAddress(FInternetAddr& OutAddr) override;

	virtual bool SetNonBlocking(bool bIsNonBlocking) override;

	virtual bool SetBroadcast(bool bAllowBroadcast) override;

	virtual bool JoinMulticastGroup(const FInternetAddr& GroupAddress) override;

	virtual bool LeaveMulticastGroup(const FInternetAddr& GroupAddress) override;

	virtual bool SetMulticastLoopback(bool bLoopback) override;

	virtual bool SetMulticastTtl(uint8 TimeToLive) override;

	virtual bool SetReuseAddr(bool bAllowReuse) override;

	virtual bool SetLinger(bool bShouldLinger, int32 Timeout) override;

	virtual bool SetRecvErr(bool bUseErrorQueue) override;

	virtual bool SetSendBufferSize(int32 Size, int32& NewSize) override;

	virtual bool SetReceiveBufferSize(int32 Size, int32& NewSize) override;

	virtual int32 GetPortNo() override;


public:
	/**
	 * Delegate for hooking 'SendTo'
	 *
	 * @param Data			The data being sent
	 * @param Count			The number of bytes being sent
	 * @param bBlockSend	Whether or not to block the send (defaults to false)
	 */
	DECLARE_DELEGATE_ThreeParams(FSendToDel, void* /*Data*/, int32 /*Count*/, bool& /*bBlockSend*/);


public:
	/** The minimal client which owns this socket hook (if any) */
	UMinimalClient* MinClient;

	/** Delegate for hooking 'SendTo' */
	FSendToDel SendToDel;

	/** Whether or not the last 'SendTo' call was blocked */
	bool bLastSendToBlocked;


public:
	/** The original socket which has been hooked */
	FSocket* HookedSocket;
};

/**
 * A delegate network notify class, to allow for easy inline-hooking.
 * 
 * NOTE: This will memory leak upon level change and re-hooking (if used as a hook),
 * because there is no consistent way to handle deleting it.
 */
class FNetworkNotifyHook : public FNetworkNotify
{
public:
	/**
	 * Base constructor
	 */
	FNetworkNotifyHook()
		: FNetworkNotify()
		, NotifyAcceptingConnectionDelegate()
		, NotifyAcceptedConnectionDelegate()
		, NotifyAcceptingChannelDelegate()
		, NotifyControlMessageDelegate()
		, HookedNotify(NULL)
	{
	}

	/**
	 * Constructor which hooks an existing network notify
	 */
	FNetworkNotifyHook(FNetworkNotify* InHookNotify)
		: FNetworkNotify()
		, HookedNotify(InHookNotify)
	{
	}

	/**
	 * Virtual destructor
	 */
	virtual ~FNetworkNotifyHook()
	{
	}


protected:
	/**
	 * Old/original notifications
	 */

	virtual EAcceptConnection::Type NotifyAcceptingConnection() override;

	virtual void NotifyAcceptedConnection(UNetConnection* Connection) override;

	virtual bool NotifyAcceptingChannel(UChannel* Channel) override;

	virtual void NotifyControlMessage(UNetConnection* Connection, uint8 MessageType, FInBunch& Bunch) override;


	DECLARE_DELEGATE_RetVal(EAcceptConnection::Type, FNotifyAcceptingConnectionDelegate);
	DECLARE_DELEGATE_OneParam(FNotifyAcceptedConnectionDelegate, UNetConnection* /*Connection*/);
	DECLARE_DELEGATE_RetVal_OneParam(bool, FNotifyAcceptingChannelDelegate, UChannel* /*Channel*/);
	DECLARE_DELEGATE_RetVal_ThreeParams(bool, FNotifyControlMessageDelegate, UNetConnection* /*Connection*/, uint8 /*MessageType*/,
										FInBunch& /*Bunch*/);

public:
	FNotifyAcceptingConnectionDelegate	NotifyAcceptingConnectionDelegate;
	FNotifyAcceptedConnectionDelegate	NotifyAcceptedConnectionDelegate;
	FNotifyAcceptingChannelDelegate		NotifyAcceptingChannelDelegate;
	FNotifyControlMessageDelegate		NotifyControlMessageDelegate;

	/** If this is hooking an existing network notify, save a reference */
	FNetworkNotify* HookedNotify;
};


/**
 * Class for hooking world tick events, and setting globals for log hooking
 */
class FWorldTickHook
{
public:
	FWorldTickHook(UWorld* InWorld)
		: AttachedWorld(InWorld)
	{
	}

	void Init()
	{
		if (AttachedWorld != NULL)
		{
#if TARGET_UE4_CL >= CL_DEPRECATEDEL
			TickDispatchDelegateHandle  = AttachedWorld->OnTickDispatch().AddRaw(this, &FWorldTickHook::TickDispatch);
			PostTickFlushDelegateHandle = AttachedWorld->OnPostTickFlush().AddRaw(this, &FWorldTickHook::PostTickFlush);
#else
			AttachedWorld->OnTickDispatch().AddRaw(this, &FWorldTickHook::TickDispatch);
			AttachedWorld->OnPostTickFlush().AddRaw(this, &FWorldTickHook::PostTickFlush);
#endif
		}
	}

	void Cleanup()
	{
		if (AttachedWorld != NULL)
		{
#if TARGET_UE4_CL >= CL_DEPRECATEDEL
			AttachedWorld->OnPostTickFlush().Remove(PostTickFlushDelegateHandle);
			AttachedWorld->OnTickDispatch().Remove(TickDispatchDelegateHandle);
#else
			AttachedWorld->OnPostTickFlush().RemoveRaw(this, &FWorldTickHook::PostTickFlush);
			AttachedWorld->OnTickDispatch().RemoveRaw(this, &FWorldTickHook::TickDispatch);
#endif
		}

		AttachedWorld = NULL;
	}

private:
	void TickDispatch(float DeltaTime)
	{
		GActiveLogWorld = AttachedWorld;
	}

	void PostTickFlush()
	{
		GActiveLogWorld = NULL;
	}

public:
	/** The world this is attached to */
	UWorld* AttachedWorld;

#if TARGET_UE4_CL >= CL_DEPRECATEDEL
private:
	/** Handle for Tick dispatch delegate */
	FDelegateHandle TickDispatchDelegateHandle;

	/** Handle for PostTick dispatch delegate */
	FDelegateHandle PostTickFlushDelegateHandle;
#endif
};


/**
 * Hooks netcode object serialization, in order to replace replication of a specific object, with another specified object,
 * for the lifetime of the scoped instance
 */
class NETCODEUNITTEST_API FScopedNetObjectReplace
{
public:
	FScopedNetObjectReplace(UClientUnitTest* InUnitTest, UObject* InObjToReplace, UObject* InObjReplacement);

	~FScopedNetObjectReplace();


private:
	UClientUnitTest* UnitTest;

	UObject* ObjToReplace;
};


/**
 * Netcode based utility functions
 */
struct NUTNet
{
	/**
	 * Gets the index of the first free/unused net channel, on the specified net connection
	 *
	 * @param InConnection	The net connection to get the index from
	 * @param InChIndex		Optionally, specifies the channel index at which to begin searching
	 * @return				Returns the first free channel index
	 */
	static int32 GetFreeChannelIndex(UNetConnection* InConnection, int32 InChIndex=INDEX_NONE);

	/**
	 * Creates a unit test channel, for capturing incoming data on a particular channel index,
	 * and routing it to a delegate for hooking or other purposes
	 *
	 * @param InConnection	The connection to create the channel in
	 * @param InType		The type of channel to create
	 * @param InChIndex		Optionally, specify the channel index
	 * @param bInVerifyOpen	Optionally, make the channel verify that it has been opened (keep sending initial packet, until ack)
	 * @return				Returns the created unit test channel
	 */
	static UUnitTestChannel* CreateUnitTestChannel(UNetConnection* InConnection, EChannelType InType, int32 InChIndex=INDEX_NONE,
													bool bInVerifyOpen=false);

	/**
	 * Outputs a bunch for the specified channel, with the ability to create the channel as well
	 * NOTE: BunchSequence should start at 0 for new channels, and should be tracked per-channel
	 * WARNING: Can return null! (e.g. if the control channel is saturated)
	 *
	 * @param BunchSequence		Specify a variable for keeping track of the bunch sequence on this channel (may need to initialize as 0)
	 * @param InConnection		Specify the connection the bunch will be sent on
	 * @param InChType			Specify the type of the channel the bunch will be sent on
	 * @param InChIndex			Optionally, specify the index of the channel to send the bunch on
	 * @param bGetNextFreeChan	Whether or not to pick the next free/unused channel, for this bunch
	 * @return					Returns the created bunch
	 */
	static NETCODEUNITTEST_API FOutBunch* CreateChannelBunch(int32& BunchSequence, UNetConnection* InConnection, EChannelType InChType,
												int32 InChIndex=INDEX_NONE, bool bGetNextFreeChan=false);

	// @todo #JohnB: Deprecate this, used during transition to minimal client code
#if 1
	static FORCEINLINE FOutBunch* CreateChannelBunch(int32* BunchSequence, UNetConnection* InConnection, EChannelType InChType,
												int32 InChIndex=INDEX_NONE, bool bGetNextFreeChan=false)
	{
		check(BunchSequence != nullptr);

		return CreateChannelBunch(*BunchSequence, InConnection, InChType, InChIndex, bGetNextFreeChan);
	}
#endif

	/**
	 * Sends a control channel bunch over a UUnitTestChannel based control channel
	 *
	 * @param InConnection		The connection to send the bunch over
	 * @param ControlChanBunch	The bunch to send over the connections control channel
	 */
	static NETCODEUNITTEST_API void SendControlBunch(UNetConnection* InConnection, FOutBunch& ControlChanBunch);


	/**
	 * Creates an instance of the unit test net driver (can be used to create multiple such drivers, for creating multiple connections)
	 *
	 * @param InWorld	The world that the net driver should be associated with
	 * @return			Returns the created net driver instance
	 */
	static NETCODEUNITTEST_API UUnitTestNetDriver* CreateUnitTestNetDriver(UWorld* InWorld);


	/**
	 * Handles setting up the client beacon once it is replicated, so that it can properly send RPC's
	 * (normally the serverside client beacon links up with the pre-existing beacon on the clientside,
	 * but with unit tests there is no pre-existing clientside beacon)
	 *
	 * @param InBeacon		The beacon that should be setup
	 * @param InConnection	The connection associated with the beacon
	 */
	static void HandleBeaconReplicate(class AOnlineBeaconClient* InBeacon, UNetConnection* InConnection);


	/**
	 * Creates a barebones/minimal UWorld, for setting up minimal fake player connections,
	 * and as a container for objects in the unit test commandlet
	 *
	 * @return	Returns the created UWorld object
	 */
	static NETCODEUNITTEST_API UWorld* CreateUnitTestWorld(bool bHookTick=true);

	/**
	 * Marks the specified unit test world for cleanup
	 *
	 * @param CleanupWorld	The unit test world to be marked for cleanup
	 * @param bImmediate	If true, all unit test worlds pending cleanup, are immediately cleaned up
	 */
	static void MarkUnitTestWorldForCleanup(UWorld* CleanupWorld, bool bImmediate=false);

	/**
	 * Cleans up unit test worlds queued for cleanup
	 */
	static void CleanupUnitTestWorlds();

	/**
	 * Returns true, if the specified world is a unit test world
	 */
	static bool IsUnitTestWorld(UWorld* InWorld);
};

