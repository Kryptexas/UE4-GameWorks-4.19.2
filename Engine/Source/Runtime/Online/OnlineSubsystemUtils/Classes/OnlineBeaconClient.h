// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//
// Basic beacon
//
#pragma once
#include "Runtime/Online/OnlineSubsystemUtils/Classes/OnlineBeacon.h"
#include "OnlineBeaconClient.generated.h"

/**
 * Base class for any actor that would like to communicate outside the normal Unreal Networking gameplay path
 */
UCLASS(transient, notplaceable, config=Engine)
class ONLINESUBSYSTEMUTILS_API AOnlineBeaconClient : public AOnlineBeacon
{
	GENERATED_UCLASS_BODY()

	// Begin AActor Interface
	void OnNetCleanup(class UNetConnection* Connection);
	// End AActor Interface

	// Begin FNetworkNotify Interface
	virtual void NotifyControlMessage(class UNetConnection* Connection, uint8 MessageType, class FInBunch& Bunch) OVERRIDE;
	// End FNetworkNotify Interface

	// Begin OnlineBeacon Interface
	virtual void DestroyBeacon() OVERRIDE;
	// End OnlineBeacon Interface

	/**
	 * Initialize the client beacon with connection endpoint
	 *	Creates the net driver and attempts to connect with the destination
	 *
	 * @param URL destination
	 *
	 * @return true if connection is being attempted, false otherwise
	 */
	bool InitClient(FURL& URL);
	
	/**
	 * A connection has been made and RPC/replication can begin
	 */
	virtual void OnConnected() {};

	/**
	 * Get the owner of this beacon actor, some host that is listening for connections
	 * (server side only, clients have no access)
	 *
	 * @return owning host of this actor
	 */
	class AOnlineBeaconHost* GetBeaconOwner() const;
	
	/**
	 * Set the owner of this beacon actor, some host that is listening for connections
	 * (server side only, clients have no access)
	 *
	 * @return owning host of this actor
	 */
	void SetBeaconOwner(class AOnlineBeaconHost* InBeaconOwner);

protected:

	/** Owning beacon host of this beacon actor */
	UPROPERTY()
	class AOnlineBeaconHost* BeaconOwner;

private:

	/**
	 * Called on the server side to open up the actor channel that will allow RPCs to occur
	 */
	UFUNCTION(client, reliable)
	virtual void ClientOnConnected();

	friend AOnlineBeaconHost;
};