// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameState.generated.h"

//=============================================================================
// GameState is replicated and is valid on servers and clients.
//=============================================================================

UCLASS(config=Game, HeaderGroup=ReplicationInfo, notplaceable, BlueprintType, Blueprintable)
class ENGINE_API AGameState : public AInfo
{
	GENERATED_UCLASS_BODY()

	/** Class of the server's game mode, assigned by GameMode. */
	UPROPERTY(replicatedUsing=OnRep_GameModeClass)
	TSubclassOf<class AGameMode>  GameModeClass;

	/** Instance of the current game mode, exists only on the server. For non-authority clients, this will be NULL. */
	UPROPERTY(Transient, BlueprintReadOnly, Category=GameState)
	class AGameMode* AuthorityGameMode;

	/** Class used by spectators, assigned by GameMode. */
	UPROPERTY(replicatedUsing=OnRep_SpectatorClass)
	TSubclassOf<class ASpectatorPawn> SpectatorClass;

	/** Match is in progress. */
	UPROPERTY(replicatedUsing=OnRep_bMatchHasBegun)
	uint32 bMatchHasBegun:1;

	/** Match is over. */
	UPROPERTY(replicatedUsing=OnRep_bMatchIsOver)
	uint32 bMatchIsOver:1;

	/** Elapsed game time since GameState was created. */
	UPROPERTY(replicated, BlueprintReadOnly, Category=GameState)
	int32 ElapsedTime;

	/** Array of all PlayerStates, maintained on both server and clients (PlayerStates are always relevant) */
	UPROPERTY(BlueprintReadOnly, Category=GameState)
	TArray<class APlayerState*> PlayerArray;

	/** This list mirrors the GameMode's list of inactive PlayerState objects. */
	UPROPERTY()
	TArray<class APlayerState*> InactivePlayerArray;

	/** GameMode class notification callback. */
	UFUNCTION()
	virtual void OnRep_GameModeClass();

	/** Callback when we receive the spectator class */
	UFUNCTION()
	virtual void OnRep_SpectatorClass();

	/** Match begun Notification Callback */
	UFUNCTION()
	virtual void OnRep_bMatchHasBegun();

	/** Match Is Over Notification Callback */
	UFUNCTION()
	virtual void OnRep_bMatchIsOver();

	// Begin AActor interface
	virtual void PostInitializeComponents() OVERRIDE;
	// End AActor interface

	/** Helper to return the default object of the GameMode class corresponding to this GameState */
	AGameMode* GetDefaultGameMode() const;
	
	/** Called when the GameClass property is set (at startup for the server, after the variable has been replicated on clients) */
	virtual void ReceivedGameModeClass();

	/** Called when the SpectatorClass property is set (at startup for the server, after the variable has been replicated on clients) */
	virtual void ReceivedSpectatorClass();

	/** Add PlayerState to the PlayerArray */
	virtual void AddPlayerState(class APlayerState* PlayerState);

	/** Remove PlayerState from the PlayerArray. */
	virtual void RemovePlayerState(class APlayerState* PlayerState);

	/** Called on the server when the match has begun. */
	virtual void StartMatch();

	/** Called on the server when the match is over. */
	virtual void EndGame();

	/** Called periodically, overridden by subclasses */
	virtual void DefaultTimer();

	/** Should players show gore? */
	virtual bool ShouldShowGore() const;

private:
	// Hidden functions that don't make sense to use on this class.
	HIDE_ACTOR_TRANSFORM_FUNCTIONS();
};



