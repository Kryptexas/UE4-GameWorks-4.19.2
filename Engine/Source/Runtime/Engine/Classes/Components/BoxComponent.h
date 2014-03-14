// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "BoxComponent.generated.h"

/** 
 * A box generally used for simple collision. Bounds are rendered as lines in the editor.
 */
UCLASS(HeaderGroup=Component, ClassGroup=Shapes, hidecategories=(Object,LOD,Lighting,TextureStreaming), editinlinenew, MinimalAPI, meta=(BlueprintSpawnableComponent))
class UBoxComponent : public UShapeComponent
{
	GENERATED_UCLASS_BODY()

protected:
	/** The extents of the box **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, export, Category=Shape)
	FVector BoxExtent;

public:
	/** 
	 * Change the box extent size. This is the unscaled size, before component scale is applied.
	 * @param	InBoxExtent: new extent for the box.
	 * @param	bUpdateOverlaps: if true and this shape is registered and collides, updates touching array for owner actor.
	 */
	UFUNCTION(BlueprintCallable, Category="Components|Box")
	ENGINE_API void SetBoxExtent(FVector InBoxExtent, bool bUpdateOverlaps=true);

	// @return the box extent, scaled by the component scale.
	UFUNCTION(BlueprintCallable, Category="Components|Box")
	ENGINE_API FVector GetScaledBoxExtent() const;

	// @return the box extent, ignoring component scale.
	UFUNCTION(BlueprintCallable, Category="Components|Box")
	ENGINE_API FVector GetUnscaledBoxExtent() const;

	// Begin UPrimitiveComponent interface.
	virtual bool IsZeroExtent() const OVERRIDE;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	virtual struct FCollisionShape GetCollisionShape(float Inflation = 0.0f) const OVERRIDE;
	// End UPrimitiveComponent interface.

	// Begin USceneComponent interface
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const OVERRIDE;
	// End USceneComponent interface

	// Begin UShapeComponent interface
	virtual void UpdateBodySetup() OVERRIDE;
	// End UShapeComponent interface

	// Sets the box extents without triggering a render or physics update.
	FORCEINLINE void InitBoxExtent(const FVector& InBoxExtent) { BoxExtent = InBoxExtent; }
};


// ----------------- INLINES ---------------

FORCEINLINE FVector UBoxComponent::GetScaledBoxExtent() const
{
	return BoxExtent * ComponentToWorld.GetScale3D();
}

FORCEINLINE FVector UBoxComponent::GetUnscaledBoxExtent() const
{
	return BoxExtent;
}
