// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "BrushComponent.generated.h"

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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Transform)
	FVector PrePivot;

	// Begin UObject interface
	virtual void PostLoad() override;
	virtual SIZE_T GetResourceSize(EResourceSizeMode::Type Mode) override;
	// End UObject interface

	// Begin USceneComponent interface
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const override;
	virtual FVector GetCustomLocation() const override;
protected:
	
	/** Calculate the new ComponentToWorld transform for this component.
	Parent is optional and can be used for computing ComponentToWorld based on arbitrary USceneComponent.
	If Parent is not passed in we use the component's AttachParent*/
	virtual FTransform CalcNewComponentToWorld(const FTransform& NewRelativeTransform, const USceneComponent * Parent = NULL) const override;
	// End USceneComponent interface

public:

	// Begin UPrimitiveComponent interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual class UBodySetup* GetBodySetup() override { return BrushBodySetup; };
	virtual void GetUsedMaterials( TArray<UMaterialInterface*>& OutMaterials ) const override;
	virtual uint8 GetStaticDepthPriorityGroup() const override;
	// End UPrimitiveComponent interface.

	/** Create the AggGeom collection-of-convex-primitives from the Brush UModel data. */
	ENGINE_API void BuildSimpleBrushCollision();
};


