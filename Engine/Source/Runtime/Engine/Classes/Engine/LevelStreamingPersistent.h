// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

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
	virtual bool ShouldBeLoaded( const FVector& ViewLocation ) override
	{
		return true;
	}
	virtual bool ShouldBeVisible( const FVector& ViewLocation ) override
	{
		return true;
	}
	// End ULevelStreaming Interface
};

