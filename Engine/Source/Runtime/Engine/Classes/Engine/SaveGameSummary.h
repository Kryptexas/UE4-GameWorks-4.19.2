// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * SaveGameSummary
 * Helper object embedded in save games containing information about saved map.
 *
 */

#pragma once
#include "SaveGameSummary.generated.h"

UCLASS(deprecated)
class UDEPRECATED_SaveGameSummary : public UObject
{
	GENERATED_UCLASS_BODY()

	/**
	 * Name of level this savegame is saved against. The level must be already in memory
	 * before the savegame can be applied.
	 */
	UPROPERTY()
	FName BaseLevel;

	/** Human readable description */
	UPROPERTY()
	FString Description;

};

