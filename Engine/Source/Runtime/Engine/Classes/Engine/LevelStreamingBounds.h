// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

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
	virtual bool ShouldBeLoaded( const FVector& ViewLocation ) override;
	virtual bool ShouldBeVisible( const FVector& ViewLocation ) override;
	// End ULevelStreaming Interface
};

