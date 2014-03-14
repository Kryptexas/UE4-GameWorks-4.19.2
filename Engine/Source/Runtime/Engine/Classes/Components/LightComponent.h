// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "LightComponent.generated.h"

/** Used to store lightmap data during RerunConstructionScripts */
class FPrecomputedLightInstanceData : public FComponentInstanceDataBase
{
public:
	static const FName PrecomputedLightInstanceDataTypeName;

	virtual ~FPrecomputedLightInstanceData()
	{}

	// Begin FComponentInstanceDataBase interface
	virtual FName GetDataTypeName() const OVERRIDE
	{
		return PrecomputedLightInstanceDataTypeName;
	}
	// End FComponentInstanceDataBase interface

	FTransform Transform;
	FGuid LightGuid;
	int32 ShadowMapChannel;
	int32 PreviewShadowMapChannel;
	bool bPrecomputedLightingIsValid;
};

UCLASS(HeaderGroup=Component, abstract, HideCategories=(Trigger,Activation,"Components|Activation",Physics), ShowCategories=(Mobility))
class ENGINE_API ULightComponent : public ULightComponentBase
{
	GENERATED_UCLASS_BODY()

	/** 
	 * Shadow map channel which is used to match up with the appropriate static shadowing during a deferred shading pass.
	 * This is generated during a lighting build.
	 */
	UPROPERTY()
	int32 ShadowMapChannel;

	/** Transient shadowmap channel used to preview the results of stationary light shadowmap packing. */
	int32 PreviewShadowMapChannel;
	
	/** 
	 * Scales the indirect lighting contribution from this light. 
	 * A value of 0 disables any GI from this light. Default is 1.
	 */
	UPROPERTY(BlueprintReadOnly, interp, Category=Light, meta=(UIMin = "0.0", UIMax = "6.0"))
	float IndirectLightingIntensity;

	/** Radius of light source shape. Moved to point light */
	UPROPERTY()
	float SourceRadius_DEPRECATED;

	/** Min roughness effective for this light. Used for softening specular highlights. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Light, AdvancedDisplay, meta=(UIMin = "0.08", UIMax = "1.0"))
	float MinRoughness;

	/** 
	 * Controls how accurate self shadowing of whole scene shadows from this light are.  
	 * At close to 0, shadows will start far from their caster, and there won't be self shadowing artifacts.
	 * At close to 1, shadows will start very close to their caster, but there will be many self shadowing artifacts.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Light, AdvancedDisplay, meta=(UIMin = "0", UIMax = "1"))
	float SelfShadowingAccuracy;

	/** 
	 * Controls how accurate self shadowing of whole scene shadows from this light are.  
	 * At 0, shadows will start at the their caster surface, but there will be many self shadowing artifacts.
	 * larger values, shadows will start further from their caster, and there won't be self shadowing artifacts but object might appear to fly.
	 * around 0.5 seems to be a good tradeoff. This also affects the soft transition of shadows
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Light, AdvancedDisplay, meta=(UIMin = "0", UIMax = "1"))
	float ShadowBias;

	/** Amount to sharpen shadow filtering */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Light, AdvancedDisplay, meta=(UIMin = "0.0", UIMax = "1.0", DisplayName = "Shadow Filter Sharpen"))
	float ShadowSharpen;
	
	UPROPERTY()
	uint32 InverseSquaredFalloff_DEPRECATED:1;

	/** Runtime toggleable enabled state - this has been replaced by USceneComponent::bVisible */
	UPROPERTY()
	uint32 bEnabled_DEPRECATED:1;

	/** Whether the light is allowed to cast dynamic shadows from translucency. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Light, AdvancedDisplay)
	uint32 CastTranslucentShadows:1;

	/**
	 * Whether the light should be injected into the Light Propagation Volume
	 **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Light, AdvancedDisplay)
	uint32 bAffectDynamicIndirectLighting : 1;

	/** 
	 * The light function material to be applied to this light.
	 * Note that only non-lightmapped lights (UseDirectLightMap=False) can have a light function. 
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=LightFunction)
	class UMaterialInterface* LightFunctionMaterial;

	/** Scales the light function projection.  X and Y scale in the directions perpendicular to the light's direction, Z scales along the light direction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=LightFunction)
	FVector LightFunctionScale;

	/** IES texture (light profiles from real world measured data) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=LightProfiles, meta=(DisplayName = "IES Texture"))
	class UTextureLightProfile* IESTexture;

	/** true: take light brightness from IES profile, false: use the light brightness - the maximum light in one direction is used to define no masking. Use with InverseSquareFalloff. Will be disabled if a valid IES profile texture is not supplied. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=LightProfiles, meta=(DisplayName = "Use IES Intensity"))
	uint32 bUseIESBrightness : 1;

	/** Global scale for IES brightness contribution. Only available when "Use IES Brightness" is selected, and a valid IES profile texture is set */
	UPROPERTY(BlueprintReadOnly, interp, Category=LightProfiles, meta=(UIMin = "0.0", UIMax = "10.0", DisplayName = "IES Intensity Scale"))
	float IESBrightnessScale;

	/** 
	 * Distance at which the light function should be completely faded to DisabledBrightness.  
	 * This is useful for hiding aliasing from light functions applied in the distance.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=LightFunction)
	float LightFunctionFadeDistance;

	/** 
	 * Brightness factor applied to the light when the light function is specified but disabled, for example in scene captures that use SceneCapView_LitNoShadows. 
	 * This should be set to the average brightness of the light function material's emissive input, which should be between 0 and 1.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=LightFunction)
	float DisabledBrightness;

	/** 
	 * Whether to render light shaft bloom from this light. 
	 * For directional lights, the color around the light direction will be blurred radially and added back to the scene.
	 * for point lights, the color on pixels closer than the light's SourceRadius will be blurred radially and added back to the scene.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=LightShafts)
	uint32 bEnableLightShaftBloom:1;

	/** Scales the additive color. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=LightShafts, meta=(UIMin = "0", UIMax = "10"))
	float BloomScale;

	/** Scene color must be larger than this to create bloom in the light shafts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=LightShafts, meta=(UIMin = "0", UIMax = "4"))
	float BloomThreshold;

	/** Multiplies against scene color to create the bloom color. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=LightShafts)
	FColor BloomTint;

public:
	/** Set brightness of the light */
	UFUNCTION(BlueprintCallable, Category="Rendering|Components|Light")
	void SetBrightness(float NewBrightness);

	/** Set color of the light */
	UFUNCTION(BlueprintCallable, Category="Rendering|Components|Light")
	void SetLightColor(FLinearColor NewLightColor);

	UFUNCTION(BlueprintCallable, Category="Rendering|Components|Light")
	void SetLightFunctionMaterial(UMaterialInterface* NewLightFunctionMaterial);

	UFUNCTION(BlueprintCallable, Category="Rendering|Components|Light")
	void SetLightFunctionScale(FVector NewLightFunctionScale);

	UFUNCTION(BlueprintCallable, Category="Rendering|Components|Light")
	void SetLightFunctionFadeDistance(float NewLightFunctionFadeDistance);

	UFUNCTION(BlueprintCallable, Category="Rendering|Components|Light")
	void SetCastShadows(bool bNewValue);

	UFUNCTION(BlueprintCallable, Category="Rendering|Components|Light")
	void SetAffectDynamicIndirectLighting(bool bNewValue);

	UFUNCTION(BlueprintCallable, Category="Rendering|Components|Light")
	void SetAffectTranslucentLighting(bool bNewValue);

	UFUNCTION(BlueprintCallable, Category="Rendering|Components|Light")
	void SetEnableLightShaftBloom(bool bNewValue);

	UFUNCTION(BlueprintCallable, Category="Rendering|Components|Light")
	void SetBloomScale(float NewValue);

	UFUNCTION(BlueprintCallable, Category="Rendering|Components|Light")
	void SetBloomThreshold(float NewValue);

	UFUNCTION(BlueprintCallable, Category="Rendering|Components|Light")
	void SetBloomTint(FColor NewValue);

	UFUNCTION(BlueprintCallable, Category="Rendering|Components|Light", meta=(FriendlyName = "Set IES Texture"))
	void SetIESTexture(UTextureLightProfile* NewValue);

public:
	/** The light's scene info. */
	class FLightSceneProxy* SceneProxy;

	/**
	 * Test whether this light affects the given primitive.  This checks both the primitive and light settings for light relevance
	 * and also calls AffectsBounds.
	 * @param PrimitiveSceneInfo - The primitive to test.
	 * @return True if the light affects the primitive.
	 */
	bool AffectsPrimitive(const UPrimitiveComponent* Primitive) const;

	/**
	 * Test whether the light affects the given bounding volume.
	 * @param Bounds - The bounding volume to test.
	 * @return True if the light affects the bounding volume
	 */
	virtual bool AffectsBounds(const FBoxSphereBounds& Bounds) const;

	/**
	 * Return the world-space bounding box of the light's influence.
	 */
	virtual FBox GetBoundingBox() const { return FBox(FVector(-HALF_WORLD_MAX,-HALF_WORLD_MAX,-HALF_WORLD_MAX),FVector(HALF_WORLD_MAX,HALF_WORLD_MAX,HALF_WORLD_MAX)); }

	virtual FSphere GetBoundingSphere() const
	{
		return FSphere(FVector::ZeroVector, HALF_WORLD_MAX);
	}

	/**
	 * Return the homogenous position of the light.
	 */
	virtual FVector4 GetLightPosition() const PURE_VIRTUAL(ULightComponent::GetPosition,return FVector4(););

	/**
	* @return ELightComponentType for the light component class
	*/
	virtual ELightComponentType GetLightType() const PURE_VIRTUAL(ULightComponent::GetLightType,return LightType_MAX;);



	/**
	 * Update/reset light GUIDs.
	 */
	virtual void UpdateLightGUIDs() OVERRIDE;

	/**
	 * Check whether a given primitive will cast shadows from this light.
	 * @param Primitive - The potential shadow caster.
	 * @return Returns True if a primitive blocks this light.
	 */
	bool IsShadowCast(UPrimitiveComponent* Primitive) const;

	/* Whether to consider light as a sunlight for atmospheric scattering. */  
	virtual bool IsUsedAsAtmosphereSunLight() const
	{
		return false;
	}

	/** Compute current light brightness based on whether there is a valid IES profile texture attached, and whether IES brightness is enabled */
	float ComputeLightBrightness() const;

	// Begin UObject interface.
	virtual void PostLoad() OVERRIDE;
#if WITH_EDITOR
	virtual void PreEditUndo() OVERRIDE;
	virtual void PostEditUndo() OVERRIDE;
	virtual bool CanEditChange(const UProperty* InProperty) const OVERRIDE;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	virtual void UpdateLightSpriteTexture() OVERRIDE;
#endif // WITH_EDITOR
	// End UObject interface.

	virtual void GetComponentInstanceData(FComponentInstanceDataCache& Cache) const OVERRIDE;
	virtual void ApplyComponentInstanceData(const FComponentInstanceDataCache& Cache) OVERRIDE;

	/** @return number of material elements in this primitive */
	virtual int32 GetNumMaterials() const;

	/** @return MaterialInterface assigned to the given material index (if any) */
	virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const;

	/** Set the MaterialInterface to use for the given element index (if valid) */
	virtual void SetMaterial(int32 ElementIndex, UMaterialInterface* InMaterial);
	

	virtual class FLightSceneProxy* CreateSceneProxy() const
	{
		return NULL;
	}

protected:
	// Begin UActorComponent Interface
	virtual void OnRegister() OVERRIDE;
	virtual void CreateRenderState_Concurrent() OVERRIDE;
	virtual void SendRenderTransform_Concurrent() OVERRIDE;
	virtual void DestroyRenderState_Concurrent() OVERRIDE;
	// Begin UActorComponent Interface

public:
	virtual void InvalidateLightingCacheDetailed(bool bInvalidateBuildEnqueuedLighting, bool bTranslationOnly) OVERRIDE;

	/** Invalidates the light's cached lighting with the option to recreate the light Guids. */
	void InvalidateLightingCacheInner(bool bRecreateLightGuids);

	/** Script interface to retrieve light direction. */
	FVector GetDirection() const;

	/** Script interface to update the color and brightness on the render thread. */
	void UpdateColorAndBrightness();

	/** 
	 * Called when property is modified by InterpPropertyTracks
	 *
	 * @param PropertyThatChanged	Property that changed
	 */
	virtual void PostInterpChange(UProperty* PropertyThatChanged);

	/** 
	 * Iterates over ALL stationary light components in the target world and assigns their preview shadowmap channel, and updates light icons accordingly.
	 * Also handles assignment after a lighting build, so that the same algorithm is used for previewing and static lighting.
	 */
	static void ReassignStationaryLightChannels(UWorld* TargetWorld, bool bAssignForLightingBuild);

};



