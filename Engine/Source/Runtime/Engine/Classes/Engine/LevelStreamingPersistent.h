// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * LevelStreamingPersistent
 *
 * @documentation
 *
 */

#pragma once
#include "LevelStreamingPersistent.generated.h"

UCLASS(transient)
class ULevelStreamingPersistent : public ULevelStreaming
{
	GENERATED_UCLASS_BODY()


	// Begin ULevelStreaming Interface
	virtual bool ShouldBeLoaded( const FVector& ViewLocation ) OVERRIDE
	{
		return true;
	}
	virtual bool ShouldBeVisible( const FVector& ViewLocation ) OVERRIDE
	{
		return true;
	}
	// End ULevelStreaming Interface
};

