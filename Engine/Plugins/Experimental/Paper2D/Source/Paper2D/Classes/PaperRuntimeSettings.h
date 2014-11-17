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

	// Test property
	UPROPERTY(GlobalConfig, EditAnywhere, Category=Settings)
	int32 TestProperty;
};
