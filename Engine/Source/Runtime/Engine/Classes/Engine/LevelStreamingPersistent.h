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
	GENERATED_BODY()
public:
	ULevelStreamingPersistent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// Begin ULevelStreaming Interface
	virtual bool ShouldBeLoaded() const override
	{
		return true;
	}
	virtual bool ShouldBeVisible() const override
	{
		return true;
	}
	// End ULevelStreaming Interface
};

