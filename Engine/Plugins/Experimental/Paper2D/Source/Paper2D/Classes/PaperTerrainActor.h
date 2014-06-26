// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"

#include "PaperTerrainActor.generated.h"

/**
 * An instance of a piece of 2D terrain in the level
 */

UCLASS(BlueprintType, Experimental)
class PAPER2D_API APaperTerrainActor : public AActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TSubobjectPtr<class USceneComponent> DummyRoot;

	UPROPERTY()
	TSubobjectPtr<class UPaperTerrainSplineComponent> SplineComponent;

	UPROPERTY(Category=Sprite, VisibleAnywhere, BlueprintReadOnly, meta = (ExposeFunctionCategories = "Sprite,Rendering,Physics,Components|Sprite"))
	TSubobjectPtr<class UPaperTerrainComponent> RenderComponent;

	// AActor interface
#if WITH_EDITOR
	virtual bool GetReferencedContentObjects(TArray<UObject*>& Objects) const override;
#endif
	// End of AActor interface
};
