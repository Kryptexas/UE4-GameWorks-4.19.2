// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * LevelStreamingAlwaysLoaded
 *
 * @documentation
 *
 */

#pragma once
#include "LevelStreamingAlwaysLoaded.generated.h"

UCLASS(MinimalAPI)
class ULevelStreamingAlwaysLoaded : public ULevelStreaming
{
	GENERATED_UCLASS_BODY()

	// Begin ULevelStreaming Interface
	virtual bool ShouldBeLoaded( const FVector& ViewLocation ) OVERRIDE;
	virtual bool ShouldBeAlwaysLoaded() const OVERRIDE { return true; } 
	// End ULevelStreaming Interface
};

