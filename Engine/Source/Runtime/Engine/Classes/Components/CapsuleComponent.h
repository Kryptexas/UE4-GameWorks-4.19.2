// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CapsuleComponent.generated.h"

/** 
 * A capsule generally used for simple collision. Bounds are rendered as lines in the editor.
 */
UCLASS(HeaderGroup=Component, ClassGroup=Shapes, editinlinenew, hidecategories=(Object,LOD,Lighting,TextureStreaming), MinimalAPI, meta=(BlueprintSpawnableComponent))
class UCapsuleComponent : public UShapeComponent
{
	GENERATED_UCLASS_BODY()

protected:
	/** Half-height, i.e. from center of capsule to end of top or bottom hemisphere.  */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, export, Category=Shape)
	float CapsuleHalfHeight;

	/** Radius of cap hemispheres and center cylinder. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, export, Category=Shape)
	float CapsuleRadius;

protected:
	UPROPERTY()
	float CapsuleHeight_DEPRECATED;

public:
	/** 
	 * Change the capsule size. This is the unscaled size, before component scale is applied.
	 * @param	InRadius : radius of end-cap hemispheres and center cylinder.
	 * @param	InHalfHeight : half-height, from capsule center to end of top or bottom hemisphere. 
	 * @param	bUpdateOverlaps: if true and this shape is registered and collides, updates touching array for owner actor.
	 */
	UFUNCTION(BlueprintCallable, Category="Components|Capsule")
	ENGINE_API void SetCapsuleSize(float InRadius, float InHalfHeight, bool bUpdateOverlaps=true);

	/**
	 * Set the capsule radius. This is the unscaled radius, before component scale is applied.
	 * If this capsule collides, updates touching array for owner actor.
	 * @param	Radius : radius of end-cap hemispheres and center cylinder.
	 * @param	bUpdateOverlaps: if true and this shape is registered and collides, updates touching array for owner actor.
	 */
	UFUNCTION(BlueprintCallable, Category="Components|Capsule")
	ENGINE_API void SetCapsuleRadius(float Radius, bool bUpdateOverlaps=true);
	
	/**
	 * Set the capsule half-height. This is the unscaled half-height, before component scale is applied.
	 * If this capsule collides, updates touching array for owner actor.
	 * @param	HalfHeight : half-height, from capsule center to end of top or bottom hemisphere. 
	 * @param	bUpdateOverlaps: if true and this shape is registered and collides, updates touching array for owner actor.
	 */
	UFUNCTION(BlueprintCallable, Category="Components|Capsule")
	ENGINE_API void SetCapsuleHalfHeight(float HalfHeight, bool bUpdateOverlaps=true);

	// Begin UObject interface
	virtual void Serialize(FArchive& Ar) OVERRIDE;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	// End UObject interface

	// Begin USceneComponent interface
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const OVERRIDE;
	virtual void CalcBoundingCylinder(float& CylinderRadius, float& CylinderHalfHeight) const OVERRIDE;
	// End USceneComponent interface

	// Begin UPrimitiveComponent interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	virtual bool IsZeroExtent() const OVERRIDE;
	virtual struct FCollisionShape GetCollisionShape(float Inflation = 0.0f) const OVERRIDE;
	virtual bool AreSymmetricRotations(const FQuat& A, const FQuat& B, const FVector& Scale3D) const OVERRIDE;
	// End UPrimitiveComponent interface.

	// Begin UShapeComponent interface
	virtual void UpdateBodySetup() OVERRIDE;
	// End UShapeComponent interface

	// @return the capsule radius scaled by the component scale.
	UFUNCTION(BlueprintCallable, Category="Components|Capsule")
	ENGINE_API float GetScaledCapsuleRadius() const;

	// @return the capsule half height scaled by the component scale.
	UFUNCTION(BlueprintCallable, Category="Components|Capsule")
	ENGINE_API float GetScaledCapsuleHalfHeight() const;

	// @return the capsule radius and half height scaled by the component scale.
	UFUNCTION(BlueprintCallable, Category="Components|Capsule")
	ENGINE_API void GetScaledCapsuleSize(float& OutRadius, float& OutHalfHeight) const;


	// @return the capsule radius, ignoring component scaling.
	UFUNCTION(BlueprintCallable, Category="Components|Capsule")
	ENGINE_API float GetUnscaledCapsuleRadius() const;

	// @return the capsule half height, ignoring component scaling.
	UFUNCTION(BlueprintCallable, Category="Components|Capsule")
	ENGINE_API float GetUnscaledCapsuleHalfHeight() const;
	
	// @return the capsule radius and half height, ignoring component scaling.
	UFUNCTION(BlueprintCallable, Category="Components|Capsule")
	ENGINE_API void GetUnscaledCapsuleSize(float& OutRadius, float& OutHalfHeight) const;

	// Get the scale used by this shape. This is a uniform scale that is the minimum of any non-uniform scaling.
	// @return the scale used by this shape.
	UFUNCTION(BlueprintCallable, Category="Components|Capsule")
	ENGINE_API float GetShapeScale() const;

	// Sets the capsule size without triggering a render or physics update.
	FORCEINLINE void InitCapsuleSize(float InRadius, float InHalfHeight)
	{
		CapsuleRadius = InRadius;
		CapsuleHalfHeight = InHalfHeight;
	}

	// Sets the capsule radius without triggering a render or physics update.
	FORCEINLINE void InitCapsuleRadius(float InRadius) { CapsuleRadius = InRadius; }

	// Sets the capsule half-height without triggering a render or physics update.
	FORCEINLINE void InitCapsuleHalfHeight(float InHalfHeight) { CapsuleHalfHeight = InHalfHeight; }
};


// ----------------- INLINES ---------------

FORCEINLINE void UCapsuleComponent::SetCapsuleRadius(float Radius, bool bUpdateOverlaps)
{
	SetCapsuleSize(Radius, GetUnscaledCapsuleHalfHeight(), bUpdateOverlaps);
}

FORCEINLINE void UCapsuleComponent::SetCapsuleHalfHeight(float HalfHeight, bool bUpdateOverlaps)
{
	SetCapsuleSize(GetUnscaledCapsuleRadius(), HalfHeight, bUpdateOverlaps);
}

FORCEINLINE float UCapsuleComponent::GetScaledCapsuleRadius() const
{
	return CapsuleRadius * GetShapeScale();
}

FORCEINLINE float UCapsuleComponent::GetScaledCapsuleHalfHeight() const
{
	return CapsuleHalfHeight * GetShapeScale();
}

FORCEINLINE void UCapsuleComponent::GetScaledCapsuleSize(float& OutRadius, float& OutHalfHeight) const
{
	const float Scale = GetShapeScale();
	OutRadius = CapsuleRadius * Scale;
	OutHalfHeight = CapsuleHalfHeight * Scale;
}


FORCEINLINE float UCapsuleComponent::GetUnscaledCapsuleRadius() const
{
	return CapsuleRadius;
}

FORCEINLINE float UCapsuleComponent::GetUnscaledCapsuleHalfHeight() const
{
	return CapsuleHalfHeight;
}

FORCEINLINE void UCapsuleComponent::GetUnscaledCapsuleSize(float& OutRadius, float& OutHalfHeight) const
{
	OutRadius = CapsuleRadius;
	OutHalfHeight = CapsuleHalfHeight;
}

FORCEINLINE float UCapsuleComponent::GetShapeScale() const
{
	return ComponentToWorld.GetMinimumAxisScale();
}

