// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LevelScriptActor.generated.h"

UCLASS(HeaderGroup=LevelScript, notplaceable, meta=(KismetHideOverrides = "ReceiveAnyDamage,ReceivePointDamage,ReceiveRadialDamage,ReceiveActorBeginOverlap,ReceiveActorEndOverlap,ReceiveHit,ReceiveDestroyed,ReceiveActorBeginCursorOver,ReceiveActorEndCursorOver,ReceiveActorOnClicked,ReceiveActorOnReleased,ReceiveActorOnInputTouchBegin,ReceiveActorOnInputTouchEnd,ReceiveActorOnInputTouchEnter,ReceiveActorOnInputTouchLeave"))
class ENGINE_API ALevelScriptActor : public AActor
{
	GENERATED_UCLASS_BODY()

	// --- Utility Functions ----------------------------
	
	/** Tries to find an event named "EventName" on all other levels, and calls it */
	UFUNCTION(BlueprintCallable, meta=(BlueprintProtected = "true"), Category="Misc")
	virtual bool RemoteEvent(FName EventName);

	/**
	 * Sets the cinematic mode on all PlayerControllers
	 *
	 * @param	bInCinematicMode	specify true if the player is entering cinematic mode; false if the player is leaving cinematic mode.
	 * @param	bHidePlayer			specify true to hide the player's pawn (only relevant if bInCinematicMode is true)
	 * @param	bAffectsHUD			specify true if we should show/hide the HUD to match the value of bCinematicMode
	 * @param	bAffectsMovement	specify true to disable movement in cinematic mode, enable it when leaving
	 * @param	bAffectsTurning		specify true to disable turning in cinematic mode or enable it when leaving
	 */
	UFUNCTION(BlueprintCallable, meta=(BlueprintProtected = "true", bHidePlayer = "true", bAffectsHUD = "true"), Category="Game|Cinematic")
	virtual void SetCinematicMode(bool bCinematicMode, bool bHidePlayer = true, bool bAffectsHUD = true, bool bAffectsMovement = false, bool bAffectsTurning = false);

	// --- Level State Functions ------------------------
	/** @todo document */
	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly)
	virtual void LevelReset();

	/**
	 * Event called on world origin changes
	 *
	 * @param	OldOrigin			Previous world origin position
	 * @param	NewOrigin			New world origin position
	 */
	UFUNCTION(BlueprintImplementableEvent)
	virtual void WorldOriginChanged(FIntPoint OldOrigin, FIntPoint NewOrigin);
	
#if WITH_EDITOR
	// Begin UObject Interface
	virtual void PostDuplicate(bool bDuplicateForPIE) OVERRIDE;
	virtual void BeginDestroy() OVERRIDE;
#endif
	virtual void PreInitializeComponents() OVERRIDE;
	// End UObject Interface

	// Begin AActor Interface
	virtual void EnableInput(class APlayerController* PlayerController) OVERRIDE;
	virtual void DisableInput(class APlayerController* PlayerController) OVERRIDE;
	// End AActor Interface

	bool InputEnabled() const { return bInputEnabled; }

private:
	UPROPERTY()
	uint32 bInputEnabled:1;
};



