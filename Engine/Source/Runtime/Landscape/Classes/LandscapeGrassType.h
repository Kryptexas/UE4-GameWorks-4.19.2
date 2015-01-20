// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LandscapeGrassType.generated.h"

UCLASS(MinimalAPI)
class ULandscapeGrassType : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Grass)
	class UStaticMesh* GrassMesh;

	/* Instances per 10 square meters. */
	UPROPERTY(EditAnywhere, Category=Grass)
	float GrassDensity;

	UPROPERTY(EditAnywhere, Category=Grass)
	float PlacementJitter;

	/* The distance where instances will begin to fade out if using a PerInstanceFadeAmount material node. 0 disables. */
	UPROPERTY(EditAnywhere, Category=Grass)
	int32 StartCullDistance;

	/**
	 * The distance where instances will have completely faded out when using a PerInstanceFadeAmount material node. 0 disables. 
	 * When the entire cluster is beyond this distance, the cluster is completely culled and not rendered at all.
	 */
	UPROPERTY(EditAnywhere, Category = Grass)
	int32 EndCullDistance;

	UPROPERTY(EditAnywhere, Category = Grass)
	bool RandomRotation;

	UPROPERTY(EditAnywhere, Category = Grass)
	bool AlignToSurface;

#if WITH_EDITOR
	// Begin UObject Interface
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	// End UObject Interface
#endif
};



