// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BrushComponent.generated.h"

struct FEngineShowFlags;
struct FConvexVolume;

/** 
 *	A brush component defines a shape that can be modified within the editor. They are used both as part of BSP building, and for volumes. 
 *	@see https://docs.unrealengine.com/latest/INT/Engine/Actors/Volumes
 *	@see https://docs.unrealengine.com/latest/INT/Engine/Actors/Brushes
 */
UCLASS(editinlinenew, MinimalAPI, hidecategories=(Physics, Lighting, LOD, Rendering, TextureStreaming, Transform, Activation, "Components|Activation"), showcategories=(Mobility, "Rendering|Material"))
class UBrushComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	class UModel* Brush;

	/** Description of collision */
	UPROPERTY()
	class UBodySetup* BrushBodySetup;

	/** Local space translation */
	UPROPERTY()
	FVector PrePivot_DEPRECATED;

	// Begin UObject interface
	virtual void PostLoad() override;
	virtual SIZE_T GetResourceSize(EResourceSizeMode::Type Mode) override;
	// End UObject interface

	// Begin USceneComponent interface
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual bool ShouldCollideWhenPlacing() const override { return true; }
	// End USceneComponent interface

public:

	// Begin UPrimitiveComponent interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual class UBodySetup* GetBodySetup() override { return BrushBodySetup; };
	virtual void GetUsedMaterials( TArray<UMaterialInterface*>& OutMaterials ) const override;
	virtual uint8 GetStaticDepthPriorityGroup() const override;
#if WITH_EDITOR
	virtual bool ComponentIsTouchingSelectionBox(const FBox& InSelBBox, const FEngineShowFlags& ShowFlags, const bool bConsiderOnlyBSP, const bool bMustEncompassEntireComponent) const override;
	virtual bool ComponentIsTouchingSelectionFrustum(const FConvexVolume& InFrustum, const FEngineShowFlags& ShowFlags, const bool bConsiderOnlyBSP, const bool bMustEncompassEntireComponent) const override;
#endif
	// End UPrimitiveComponent interface.

	/** Create the AggGeom collection-of-convex-primitives from the Brush UModel data. */
	ENGINE_API void BuildSimpleBrushCollision();
};


