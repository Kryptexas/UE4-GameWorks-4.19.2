// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperTerrainActor.generated.h"

/**
 * An instance of a piece of 2D terrain in the level
 */

UCLASS(DependsOn=UEngineTypes, BlueprintType, meta=(DisplayThumbnail = "true"))
class PAPER2D_API APaperTerrainActor : public AActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TSubobjectPtr<class USceneComponent> DummyRoot;

	UPROPERTY()
	TSubobjectPtr<class UPaperTerrainSplineComponent> SplineComponent;

	UPROPERTY(Category = Sprite, VisibleAnywhere)
	TSubobjectPtr<class UPaperTerrainComponent> RenderComponent;
};
