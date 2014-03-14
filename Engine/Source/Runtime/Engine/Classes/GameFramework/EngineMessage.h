// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EngineMessage.generated.h"

UCLASS(abstract, MinimalAPI)
class  UEngineMessage : public ULocalMessage
{
	GENERATED_UCLASS_BODY()

	/** Message displayed in message dialog when player pawn fails to spawn because no playerstart was available. */
	UPROPERTY(localized)
	FString FailedPlaceMessage;

	/** Message when player join attempt is refused because the server is at capacity. */
	UPROPERTY(localized)
	FString MaxedOutMessage;

	/** Message when a new player enters the game. */
	UPROPERTY(localized)
	FString EnteredMessage;

	/** Message when a player leaves the game. */
	UPROPERTY(localized)
	FString LeftMessage;

	/** Message when a player changes his name. */
	UPROPERTY(localized)
	FString GlobalNameChange;

	/** Message when a new spectator enters the server (if spectator has a player name). */
	UPROPERTY(localized)
	FString SpecEnteredMessage;

	/** Message when a new player enters the server (if player is unnamed). */
	UPROPERTY(localized)
	FString NewPlayerMessage;

	/** Message when a new spectator enters the server (if spectator is unnamed). */
	UPROPERTY(localized)
	FString NewSpecMessage;

	// Begin ULocalMessage interface
	virtual void ClientReceive(const FClientReceiveData& ClientData) const OVERRIDE;
	// End ULocalMessage interface
};
