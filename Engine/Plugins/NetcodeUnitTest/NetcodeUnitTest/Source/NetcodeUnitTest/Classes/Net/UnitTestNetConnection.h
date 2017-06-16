// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Engine/NetDriver.h"
#include "Engine/NetConnection.h"
#include "IpConnection.h"

#include "NetcodeUnitTest.h"

#include "Net/NUTUtilNet.h"

#include "UnitTestNetConnection.generated.h"


// Forward declarations

class UMinimalClient;


/**
 * A net connection for enabling unit testing through barebones/minimal client connections
 */
UCLASS(transient)
class NETCODEUNITTEST_API UUnitTestNetConnection : public UIpConnection
{
	GENERATED_UCLASS_BODY()


	virtual void InitBase(UNetDriver* InDriver, class FSocket* InSocket, const FURL& InURL, EConnectionState InState,
							int32 InMaxPacket=0, int32 InPacketOverhead=0) override;

#if TARGET_UE4_CL < CL_INITCONNPARAM
	virtual void InitConnection(UNetDriver* InDriver, EConnectionState InState, const FURL& InURL, int32 InConnectionSpeed=0) override;
#else
	virtual void InitConnection(UNetDriver* InDriver, EConnectionState InState, const FURL& InURL, int32 InConnectionSpeed=0,
								int32 InMaxPacket=0) override;
#endif

	virtual void InitHandler() override;


	virtual void LowLevelSend(void* Data, int32 CountBytes, int32 CountBits) override;

	virtual void ValidateSendBuffer() override;

	virtual void ReceivedRawPacket(void* Data, int32 Count) override;

	virtual void HandleClientPlayer(class APlayerController* PC, class UNetConnection* NetConnection) override;


	/**
	 * Delegate for hooking the HandlerClientPlayer event
	 *
	 * @param PC			The PlayerController being initialized with the net connection
	 * @param Connection	The net connection the player is being initialized with
	 */
	DECLARE_DELEGATE_TwoParams(FNotifyHandleClientPlayerDelegate, APlayerController* /*PC*/, UNetConnection* /*Connection*/);


public:
	/** The minimal client which owns this net connection */
	UMinimalClient* MinClient;


	/** Hook for HandleClientPlayer event */
	FNotifyHandleClientPlayerDelegate	NotifyHandleClientPlayerDel;

	/** Socket hook - for hooking socket-level events */
	FSocketHook							SocketHook;

	/** Whether or not to override error detection within ValidateSendBuffer */
	bool								bDisableValidateSend;


public:
	/** Whether or not newly-created instances of this class, should force-enable packet handlers */
	static bool							bForceEnableHandler;
};





