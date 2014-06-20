// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SpriteDrawCall.h"
#include "PaperTerrainComponent.generated.h"

struct FPaperTerrainMaterialPair
{
	TArray<FSpriteDrawCallRecord> Records;
	UMaterialInterface* Material;
};

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
	// The color of the terrain (passed to the sprite material as a vertex color)
	UPROPERTY(Category=Sprite, BlueprintReadOnly, Interp)
	FLinearColor TerrainColor;

	/** Number of steps per spline segment to place in the reparameterization table */
	UPROPERTY(EditAnywhere, Category=Sprite, meta=(ClampMin=4, UIMin=4))
	int32 ReparamStepsPerSegment;

public:
	// UObject interface
	virtual const UObject* AdditionalStatObject() const override;
#if WITH_EDITORONLY_DATA
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	// End of UObject interface

	// UActorComponent interface
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	// End of UActorComponent interface

	// UPrimitiveComponent interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	// End of UPrimitiveComponent interface

	// Set color of the terrain
	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void SetTerrainColor(FLinearColor NewColor);

protected:
	void SpawnSegment(const class UPaperSprite* NewSprite, float Position, float HorizontalScale);
	void SpawnFromPoly(const class UPaperSprite* NewSprite, FSpriteDrawCallRecord& FillDrawCall, const FVector2D& TextureSize, FPoly& Poly);

	void OnSplineEdited();

	TArray<FPaperTerrainMaterialPair> GeneratedSpriteGeometry;
	
	FTransform GetTransformAtDistance(float InDistance) const;
};

