// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Runtime/Online/OnlineSubsystemUtils/Classes/OnlineBeaconClient.h"
#include "TestBeaconClient.generated.h"

/**
 * A beacon client used for making reservations with an existing game session
 */
UCLASS(transient, notplaceable, config=Engine)
class ONLINESUBSYSTEMUTILS_API ATestBeaconClient : public AOnlineBeaconClient
{
	GENERATED_BODY()
public:
	ATestBeaconClient(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Begin AOnlineBeacon Interface
	virtual FString GetBeaconType() override { return TEXT("TestBeacon"); }
	// End AOnlineBeacon Interface

	// Begin AOnlineBeaconClient Interface
	virtual void OnFailure() override;
	// End AOnlineBeaconClient Interface

	/** Send a ping RPC to the client */
	UFUNCTION(client="ClientPing_Implementation", reliable)
	virtual void ClientPing();
	virtual void ClientPing_Implementation();

	/** Send a pong RPC to the host */
	UFUNCTION(server="ServerPong_Implementation", reliable, WithValidation="ServerPong_Validate")
	virtual void ServerPong();
	virtual void ServerPong_Implementation();
	virtual bool ServerPong_Validate();
};
