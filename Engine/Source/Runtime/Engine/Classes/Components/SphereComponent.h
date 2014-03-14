// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "SphereComponent.generated.h"

/** 
 * A sphere generally used for simple collision. Bounds are rendered as lines in the editor.
 */
UCLASS(HeaderGroup=Component, ClassGroup=Shapes, editinlinenew, hidecategories=(Object,LOD,Lighting,TextureStreaming), MinimalAPI, meta=(BlueprintSpawnableComponent))
class USphereComponent : public UShapeComponent
{
	GENERATED_UCLASS_BODY()

protected:
	/** The radius of the sphere **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, export, Category=Shape)
	float SphereRadius;

public:
	/**
	 * Change the sphere radius. This is the unscaled radius, before component scale is applied.
	 * @param	InSphereRadius: the new sphere radius
	 * @param	bUpdateOverlaps: if true and this shape is registered and collides, updates touching array for owner actor.
	 */
	UFUNCTION(BlueprintCallable, Category="Components|Sphere")
	ENGINE_API void SetSphereRadius(float InSphereRadius, bool bUpdateOverlaps=true);

	// @return the radius of the sphere, with component scale applied.
	UFUNCTION(BlueprintCallable, Category="Components|Sphere")
	ENGINE_API float GetScaledSphereRadius() const;

	// @return the radius of the sphere, ignoring component scale.
	UFUNCTION(BlueprintCallable, Category="Components|Sphere")
	ENGINE_API float GetUnscaledSphereRadius() const;

	// Begin UPrimitiveComponent interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	virtual bool IsZeroExtent() const OVERRIDE;
	virtual struct FCollisionShape GetCollisionShape(float Inflation = 0.0f) const OVERRIDE;
	virtual bool AreSymmetricRotations(const FQuat& A, const FQuat& B, const FVector& Scale3D) const OVERRIDE;
	// End UPrimitiveComponent interface.

	// Begin USceneComponent interface
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const OVERRIDE;
	virtual void CalcBoundingCylinder(float& CylinderRadius, float& CylinderHalfHeight) const OVERRIDE;
	// End USceneComponent interface

	// Begin UShapeComponent interface
	virtual void UpdateBodySetup() OVERRIDE;
	// End UShapeComponent interface

	// Get the scale used by this shape. This is a uniform scale that is the minimum of any non-uniform scaling.
	// @return the scale used by this shape.
	UFUNCTION(BlueprintCallable, Category="Components|Sphere")
	ENGINE_API float GetShapeScale() const;

	// Sets the sphere radius without triggering a render or physics update.
	FORCEINLINE void InitSphereRadius(float InSphereRadius) { SphereRadius = InSphereRadius; }
};


// ----------------- INLINES ---------------

FORCEINLINE float USphereComponent::GetScaledSphereRadius() const
{
	return SphereRadius * GetShapeScale();
}

FORCEINLINE float USphereComponent::GetUnscaledSphereRadius() const
{
	return SphereRadius;
}

FORCEINLINE float USphereComponent::GetShapeScale() const
{
	return ComponentToWorld.GetMinimumAxisScale();
}
