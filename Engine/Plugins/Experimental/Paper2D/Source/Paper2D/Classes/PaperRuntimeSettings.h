// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperRuntimeSettings.generated.h"

/**
 * Implements the settings for the Paper2D plugin.
 */
UCLASS(config = Engine, defaultconfig)
class PAPER2D_API UPaperRuntimeSettings : public UObject
{
	GENERATED_UCLASS_BODY()

	// The default scaling factor between pixels and Unreal units (cm) to use for newly created sprite assets (e.g., 0.64 would make a 64 pixel wide sprite take up 100 cm)
	UPROPERTY(GlobalConfig, EditAnywhere, Category=Settings)
	float DefaultPixelsPerUnrealUnit;

	// Enables experimental *incomplete and unsupported* texture atlas groups that sprites can be assigned to
	UPROPERTY(EditAnywhere, config, Category=Experimental)
	bool bEnableSpriteAtlasGroups;
};
