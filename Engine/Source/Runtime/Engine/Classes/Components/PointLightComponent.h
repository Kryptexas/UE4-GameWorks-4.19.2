// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "PointLightComponent.generated.h"

/**
 * A light component which emits light from a single point equally in all directions.
 */
UCLASS(HeaderGroup=Light, ClassGroup=Lights, hidecategories=(Object, LightShafts), editinlinenew, meta=(BlueprintSpawnableComponent))
class ENGINE_API UPointLightComponent : public ULightComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	float Radius_DEPRECATED;

	/**
	 * Bounds the light's visible influence.  
	 * This clamping of the light's influence is not physically correct but very important for performance, larger lights cost more.
	 */
	UPROPERTY(interp, BlueprintReadOnly, Category=Light, meta=(UIMin = "8.0", UIMax = "16384.0"))
	float AttenuationRadius;

	/** 
	 * Whether to use physically based inverse squared distance falloff, where AttenuationRadius is only clamping the light's contribution.  
	 * Disabling inverse squared falloff can be useful when placing fill lights (don't want a super bright spot near the light).
	 * When enabled, the light's Intensity is in units of lumens, where 1700 lumens is a 100W lightbulb.
	 * When disabled, the light's Intensity is a brightness scale.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Light, AdvancedDisplay)
	uint32 bUseInverseSquaredFalloff:1;

	/**
	 * Controls the radial falloff of the light when UseInverseSquaredFalloff is disabled. 
	 * 2 is almost linear and very unrealistic and around 8 it looks reasonable.
	 * With large exponents, the light has contribution to only a small area of its influence radius but still costs the same as low exponents.
	 */
	UPROPERTY(interp, BlueprintReadOnly, Category=Light, AdvancedDisplay, meta=(UIMin = "2.0", UIMax = "16.0"))
	float LightFalloffExponent;

	/** 
	 * Radius of light source shape.
	 * Note that light sources shapes which intersect shadow casting geometry can cause shadowing artifacts.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Light)
	float SourceRadius;

	/** 
	 * Length of light source shape.
	 * Note that light sources shapes which intersect shadow casting geometry can cause shadowing artifacts.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Light)
	float SourceLength;

	/** The Lightmass settings for this object. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Light, meta=(ShowOnlyInnerProperties))
	struct FLightmassPointLightSettings LightmassSettings;

	UFUNCTION(BlueprintCallable, Category="Rendering|Lighting")
	void SetRadius(float NewRadius);

	UFUNCTION(BlueprintCallable, Category="Rendering|Lighting")
	void SetLightFalloffExponent(float NewLightFalloffExponent);

	UFUNCTION(BlueprintCallable, Category="Rendering|Lighting")
	void SetSourceRadius(float bNewValue);

protected:
	// Begin UActorComponent Interface
	virtual void SendRenderTransform_Concurrent() OVERRIDE;
	// End UActorComponent Interface

public:

	// ULightComponent interface.
	virtual bool AffectsBounds(const FBoxSphereBounds& Bounds) const;
	virtual FVector4 GetLightPosition() const;
	virtual FBox GetBoundingBox() const;
	virtual FSphere GetBoundingSphere() const;
	virtual ELightComponentType GetLightType() const;
	virtual FLightSceneProxy* CreateSceneProxy() const OVERRIDE;

	// Begin UObject Interface
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	virtual void PostLoad() OVERRIDE;
#if WITH_EDITOR
	virtual bool CanEditChange(const UProperty* InProperty) const OVERRIDE;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	// End UObject Interface

	/** 
	 * This is called when property is modified by InterpPropertyTracks
	 *
	 * @param PropertyThatChanged	Property that changed
	 */
	virtual void PostInterpChange(UProperty* PropertyThatChanged);

private:

	/** Pushes the value of radius to the rendering thread. */
	void PushRadiusToRenderThread();
};



