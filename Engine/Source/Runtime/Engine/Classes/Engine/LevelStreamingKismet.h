// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * LevelStreamingKismet
 *
 * Kismet triggerable streaming implementation.
 *
 */

#pragma once
#include "LevelStreamingKismet.generated.h"

UCLASS(MinimalAPI, BlueprintType)
class ULevelStreamingKismet : public ULevelStreaming
{
	GENERATED_UCLASS_BODY()

	/** Whether the level should be loaded at startup																			*/
	UPROPERTY(Category=LevelStreaming, EditAnywhere)
	uint32 bInitiallyLoaded:1;

	/** Whether the level should be visible at startup if it is loaded 															*/
	UPROPERTY(Category=LevelStreaming, EditAnywhere)
	uint32 bInitiallyVisible:1;
	
	// Begin UObject Interface
	virtual void PostLoad() OVERRIDE;
	// End UObject Interface

	// Begin ULevelStreaming Interface
	virtual bool ShouldBeLoaded( const FVector& ViewLocation ) OVERRIDE;
	virtual bool ShouldBeVisible( const FVector& ViewLocation ) OVERRIDE;
	// End ULevelStreaming Interface
};

