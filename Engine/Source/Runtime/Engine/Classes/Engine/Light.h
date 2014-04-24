// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Light.generated.h"

UCLASS(Abstract, HeaderGroup=Light, ClassGroup=Lights, hidecategories=(Input,Collision,Replication), ConversionRoot, meta=(ChildCanTick))
class ENGINE_API ALight : public AActor
{
	GENERATED_UCLASS_BODY()

	/** @todo document */
	UPROPERTY(Category=Light, VisibleAnywhere, BlueprintReadOnly, meta=(ExposeFunctionCategories="Light,Rendering,Rendering|Components|Light"))
	TSubobjectPtr<class ULightComponent> LightComponent;

	/** replicated copy of LightComponent's bEnabled property */
	UPROPERTY(replicatedUsing=OnRep_bEnabled)
	uint32 bEnabled:1;

	/** Replication Notification Callbacks */
	UFUNCTION()
	virtual void OnRep_bEnabled();

	/** Function to change mobility type of light */
	void SetMobility(EComponentMobility::Type InMobility);

	// BEGIN DEPRECATED (use component functions now in level script)
	UFUNCTION(BlueprintCallable, Category="Rendering|Lighting", meta=(DeprecatedFunction))
	void SetEnabled(bool bSetEnabled);
	UFUNCTION(BlueprintCallable, Category="Rendering|Lighting", meta=(DeprecatedFunction))
	bool IsEnabled() const;
	UFUNCTION(BlueprintCallable, Category="Rendering|Lighting", meta=(DeprecatedFunction))
	void ToggleEnabled();
	UFUNCTION(BlueprintCallable, Category="Rendering|Lighting", meta=(DeprecatedFunction))
	void SetBrightness(float NewBrightness);
	UFUNCTION(BlueprintCallable, Category="Rendering|Lighting", meta=(DeprecatedFunction))
	float GetBrightness() const;
	UFUNCTION(BlueprintCallable, Category="Rendering|Lighting", meta=(DeprecatedFunction))
	void SetLightColor(FLinearColor NewLightColor);
	UFUNCTION(BlueprintCallable, Category="Rendering|Lighting", meta=(DeprecatedFunction))
	FLinearColor GetLightColor() const;
	UFUNCTION(BlueprintCallable, Category="Rendering|Lighting", meta=(DeprecatedFunction))
	void SetLightFunctionMaterial(UMaterialInterface* NewLightFunctionMaterial);
	UFUNCTION(BlueprintCallable, Category="Rendering|Lighting", meta=(DeprecatedFunction))
	void SetLightFunctionScale(FVector NewLightFunctionScale);
	UFUNCTION(BlueprintCallable, Category="Rendering|Lighting", meta=(DeprecatedFunction))
	void SetLightFunctionFadeDistance(float NewLightFunctionFadeDistance);
	UFUNCTION(BlueprintCallable, Category="Rendering|Lighting", meta=(DeprecatedFunction))
	void SetCastShadows(bool bNewValue);
	UFUNCTION(BlueprintCallable, Category="Rendering|Lighting", meta=(DeprecatedFunction))
	void SetAffectTranslucentLighting(bool bNewValue);
	// END DEPRECATED

public:
#if WITH_EDITOR
	// Begin AActor interface.
	virtual void CheckForErrors() OVERRIDE;
	// End AActor interface.
#endif

	/**
	 * Return whether the light supports being toggled off and on on-the-fly.
	 */
	bool IsToggleable() const
	{
		return !LightComponent->HasStaticLighting();
	}

	// Begin AActor interface.
	void Destroyed();
	virtual bool IsLevelBoundsRelevant() const OVERRIDE { return false; }
	// End AActor interface.
};



