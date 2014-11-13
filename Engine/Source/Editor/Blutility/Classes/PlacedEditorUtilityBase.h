// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Base class of all placed Blutility editor utilities.
 */

#pragma once

#include "PlacedEditorUtilityBase.generated.h"

UCLASS(Abstract, hideCategories=(Object, Actor)/*, Blueprintable*/)
class BLUTILITY_API APlacedEditorUtilityBase : public AActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Category=Config, EditDefaultsOnly, BlueprintReadWrite)
	FString HelpText;

	// AActor interface
	virtual void TickActor(float DeltaSeconds, ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
	// End of AActor interface

	/**
	 * Returns the current selection set in the editor.  Note that for non-editor builds, this will return an empty array
	 */
	UFUNCTION(BlueprintPure, Category = "Development|Editor")
	TArray<AActor*> GetSelectionSet();

	/**
	 * Gets information about the camera position for the primary level editor viewport.  In non-editor builds, these will be zeroed
	 *
	 * @param	CameraLocation	(out) Current location of the level editing viewport camera, or zero if none found
	 * @param	CameraRotation	(out) Current rotation of the level editing viewport camera, or zero if none found
	 * @return	Whether or not we were able to get a camera for a level editing viewport
	 */
	UFUNCTION(BlueprintPure, Category = "Development|Editor")
	bool GetLevelViewportCameraInfo(FVector& CameraLocation, FRotator& CameraRotation);
};
