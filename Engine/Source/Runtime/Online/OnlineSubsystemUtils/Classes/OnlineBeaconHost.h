// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//
// Basic beacon
//
#pragma once
#include "Runtime/Online/OnlineSubsystemUtils/Classes/OnlineBeacon.h"
#include "OnlineBeaconHost.generated.h"

/**
 * Base class for any actor that would like to communicate outside the normal Unreal Networking gameplay path
 *
 * The host beacon listens for connections and is responsible for spawning the actual actor for communication host side
 * This actor was initiated by the client, the host verifies the validity and accepts/continues the connection
 */
UCLASS(transient, notplaceable, config=Engine)
class ONLINESUBSYSTEMUTILS_API AOnlineBeaconHost : public AOnlineBeacon
{
	GENERATED_UCLASS_BODY()

	/** Custom name for this beacon */
	UPROPERTY(Transient)
	FString BeaconName;

	/** Configured listen port for this beacon host */
	UPROPERTY(Config)
	int32 ListenPort;

	/** List of client beacon actors with active connections */
	UPROPERTY()
	TArray<class AOnlineBeaconClient*> ClientActors;

	/** Delegate to route a connection event to the appropriate beacon host, by type */
	DECLARE_DELEGATE_TwoParams(FOnBeaconConnected, class AOnlineBeaconClient*, class UNetConnection*);
	FOnBeaconConnected& OnBeaconConnected(FName BeaconType);

	/** Mapping of beacon types to the OnBeaconConnected delegates */
	TMap<FName, FOnBeaconConnected> OnBeaconConnectedMapping;

	// Begin AActor Interface
	virtual void OnNetCleanup(class UNetConnection* Connection) OVERRIDE;
	// End AActor Interface

	// Begin OnlineBeacon Interface
	virtual void HandleNetworkFailure(UWorld* World, class UNetDriver *NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString) OVERRIDE;
	// End OnlineBeacon Interface

	// Begin FNetworkNotify Interface
	virtual void NotifyControlMessage(class UNetConnection* Connection, uint8 MessageType, class FInBunch& Bunch) OVERRIDE;
	// End FNetworkNotify Interface

	/**
	 * Initialize the host beacon on a specified port
	 *	Creates the net driver and begins listening for connections
	 *
	 * @return true if host was setup correctly, false otherwise
	 */
	virtual bool InitHost();

	/**
	 * Get the listen port for this beacon
	 *
	 * @return beacon listen port
	 */
	virtual int32 GetListenPort() { return ListenPort; }

	/**
	 * Get a client beacon actor for a given connection
	 *
	 * @param Connection connection of interest
	 *
	 * @return client beacon actor that owns the connection
	 */
	virtual class AOnlineBeaconClient* GetClientActor(class UNetConnection* Connection);

	/**
	 * Remove a client beacon actor from the list of active connections
	 *
	 * @param ClientActor client beacon actor to remove
	 */
	virtual void RemoveClientActor(class AOnlineBeaconClient* ClientActor);

	/**
	 * Each beacon host must be able to spawn the appropriate client beacon actor to communicate with the initiating client
	 *
	 * @return new client beacon actor that this beacon host knows how to communicate with
	 */
	virtual class AOnlineBeaconClient* SpawnBeaconActor() PURE_VIRTUAL(AOnlineBeaconHost::SpawnBeaconActor, return NULL;);
};