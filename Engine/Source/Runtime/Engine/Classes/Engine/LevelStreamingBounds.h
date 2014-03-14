// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * LevelStreamingBounds
 *
 * Level bounds based streaming triggering
 *
 */

#pragma once
#include "LevelStreamingBounds.generated.h"

UCLASS(MinimalAPI)
class ULevelStreamingBounds : public ULevelStreaming
{
	GENERATED_UCLASS_BODY()


	// Begin ULevelStreaming Interface
	virtual bool ShouldBeLoaded( const FVector& ViewLocation ) OVERRIDE;
	virtual bool ShouldBeVisible( const FVector& ViewLocation ) OVERRIDE;
	// End ULevelStreaming Interface
};

