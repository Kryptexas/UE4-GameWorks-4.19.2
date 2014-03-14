// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Runtime/Online/OnlineSubsystemUtils/Classes/OnlineBeaconHost.h"
#include "TestBeaconHost.generated.h"

/**
 * A beacon host used for taking reservations for an existing game session
 */
UCLASS(transient, notplaceable, config=Engine)
class ONLINESUBSYSTEMUTILS_API ATestBeaconHost : public AOnlineBeaconHost
{
	GENERATED_UCLASS_BODY()

	// Begin AOnlineBeaconHost Interface 
	virtual bool InitHost() OVERRIDE;
	virtual AOnlineBeaconClient* SpawnBeaconActor() OVERRIDE;
	// End AOnlineBeaconHost Interface 

	/**
	 * Delegate triggered when a new client connection is made
	 *
	 * @param NewClientActor new client beacon actor
	 * @param ClientConnection new connection established
	 */
	virtual void ClientConnected(class AOnlineBeaconClient* NewClientActor, class UNetConnection* ClientConnection);
};
