// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "BrushComponent.generated.h"

UCLASS(HeaderGroup=Component, editinlinenew, MinimalAPI, showcategories=(Mobility), hidecategories=(Physics, Lighting, LOD, Rendering, TextureStreaming, Transform, Activation, "Components|Activation"))
class UBrushComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	class UModel* Brush;

	/** Description of collision */
	UPROPERTY()
	class UBodySetup* BrushBodySetup;

	/** Local space translation */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Transform)
	FVector PrePivot;

	// Begin UObject interface
	virtual void PostLoad() OVERRIDE;
	virtual SIZE_T GetResourceSize(EResourceSizeMode::Type Mode) OVERRIDE;
	// End UObject interface

	// Begin USceneComponent interface
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const OVERRIDE;
	virtual FVector GetCustomLocation() const OVERRIDE;
protected:
	virtual FTransform CalcNewComponentToWorld(const FTransform& NewRelativeTransform) const OVERRIDE;
	// End USceneComponent interface

public:

	// Begin UPrimitiveComponent interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	virtual class UBodySetup* GetBodySetup() OVERRIDE { return BrushBodySetup; };
	virtual void GetUsedMaterials( TArray<UMaterialInterface*>& OutMaterials ) const OVERRIDE;
	virtual uint8 GetStaticDepthPriorityGroup() const OVERRIDE;
	// End UPrimitiveComponent interface.

	/** Create the AggGeom collection-of-convex-primitives from the Brush UModel data. */
	ENGINE_API void BuildSimpleBrushCollision();
};


