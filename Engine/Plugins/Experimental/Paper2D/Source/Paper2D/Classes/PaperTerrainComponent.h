// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperTerrainComponent.generated.h"

/**
 * The terrain visualization component for an associated spline component.
 * This takes a 2D terrain material and instances sprite geometry along the spline path.
 */

UCLASS(BlueprintType)
class PAPER2D_API UPaperTerrainComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(Category=Sprite, EditAnywhere, BlueprintReadOnly)
	class UPaperTerrainMaterial* TerrainMaterial;

	UPROPERTY(Category = Sprite, EditAnywhere, BlueprintReadOnly)
	bool bClosedSpline;

	UPROPERTY(Category = Sprite, EditAnywhere, BlueprintReadOnly)
	float TestScaleFactor;

	UPROPERTY()
	class UPaperTerrainSplineComponent* AssociatedSpline;

	/** Random seed used for choosing which spline meshes to use. */
	UPROPERTY(Category=Sprite, EditAnywhere)
	int32 RandomSeed;

protected:

	UPROPERTY(Transient)
	TArray<class UPaperRenderComponent*> SpawnedComponents;

public:
	// UObject interface
#if WITH_EDITORONLY_DATA
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	// End of UObject interface

	// UActorComponent interface
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	// End of UActorComponent interface

protected:
	void SpawnSegment(class UPaperSprite* NewSprite, float Direction, float& RemainingSegStart, float& RemainingSegEnd);

	void OnSplineEdited();
	void DestroyExistingStuff();
};

