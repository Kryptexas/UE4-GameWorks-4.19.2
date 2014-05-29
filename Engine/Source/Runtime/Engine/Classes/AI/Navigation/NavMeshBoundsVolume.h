// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * this volume only blocks the path builder - it has no gameplay collision
 *
 */

#pragma once
#include "GameFramework/Volume.h"
#include "NavMeshBoundsVolume.generated.h"

UCLASS(MinimalAPI)
class ANavMeshBoundsVolume : public AVolume
{
	GENERATED_UCLASS_BODY()

	// Begin AActor Interface
	virtual void PostInitializeComponents() OVERRIDE;
#if WITH_EDITOR
	virtual void ReregisterAllComponents() OVERRIDE;
	// End AActor Interface
	// Begin UObject Interface
	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	// End UObject Interface

#endif // WITH_EDITOR
};



