// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Base class of all placed Blutility editor utilities.
 */

#pragma once

#include "PlacedEditorUtilityBase.generated.h"

UCLASS(Abstract, hideCategories=(Object, Actor)/*, Blueprintable*/)
class APlacedEditorUtilityBase : public AActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Category=Config, EditDefaultsOnly, BlueprintReadWrite)
	FString HelpText;

	// AActor interface
	virtual void TickActor(float DeltaSeconds, ELevelTick TickType, FActorTickFunction& ThisTickFunction) OVERRIDE;
	// End of AActor interface
};
