// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ExponentialHeightFogComponent.generated.h"

/**
 *	Used to create fogging effects such as clouds but with a density that is related to the height of the fog.
 */
UCLASS(HeaderGroup=Component, ClassGroup=Rendering, collapsecategories, hidecategories=(Object, Mobility), editinlinenew, meta=(BlueprintSpawnableComponent),MinimalAPI)
class UExponentialHeightFogComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

	/** True if the fog is enabled. */
	UPROPERTY()
	uint32 bEnabled_DEPRECATED:1;

	/** Global density factor. */
	UPROPERTY(BlueprintReadOnly, interp, Category=ExponentialHeightFogComponent)
	float FogDensity;

	UPROPERTY(BlueprintReadOnly, interp, Category=ExponentialHeightFogComponent)
	FLinearColor FogInscatteringColor;

	/** Controls the size of the directional inscattering cone, which is used to approximate inscattering from a directional light. */
	UPROPERTY(BlueprintReadOnly, interp, Category=DirectionalInscattering)
	float DirectionalInscatteringExponent;

	/** Controls the start distance from the viewer of the directional inscattering, which is used to approximate inscattering from a directional light. */
	UPROPERTY(BlueprintReadOnly, interp, Category=DirectionalInscattering)
	float DirectionalInscatteringStartDistance;

	/** Controls the color of the directional inscattering, which is used to approximate inscattering from a directional light. */
	UPROPERTY(BlueprintReadOnly, interp, Category=DirectionalInscattering)
	FLinearColor DirectionalInscatteringColor;

	/** 
	 * Height density factor, controls how the density increases as height decreases.  
	 * Smaller values make the visible transition larger.
	 */
	UPROPERTY(BlueprintReadOnly, interp, Category=ExponentialHeightFogComponent)
	float FogHeightFalloff;

	/** 
	 * Maximum opacity of the fog.  
	 * A value of 1 means the fog can become fully opaque at a distance and replace scene color completely,
	 * A value of 0 means the fog color will not be factored in at all.
	 */
	UPROPERTY(BlueprintReadOnly, interp, Category=ExponentialHeightFogComponent)
	float FogMaxOpacity;

	/** Distance from the camera that the fog will start, in world units. */
	UPROPERTY(BlueprintReadOnly, interp, Category=ExponentialHeightFogComponent)
	float StartDistance;

	
public:
	UFUNCTION(BlueprintCallable, Category="Rendering|Components|ExponentialHeightFog")
	void SetFogDensity(float Value);

	UFUNCTION(BlueprintCallable, Category="Rendering|Components|ExponentialHeightFog")
	void SetFogInscatteringColor(FLinearColor Value);

	UFUNCTION(BlueprintCallable, Category="Rendering|Components|ExponentialHeightFog")
	void SetDirectionalInscatteringExponent(float Value);

	UFUNCTION(BlueprintCallable, Category="Rendering|Components|ExponentialHeightFog")
	void SetDirectionalInscatteringStartDistance(float Value);

	UFUNCTION(BlueprintCallable, Category="Rendering|Components|ExponentialHeightFog")
	void SetDirectionalInscatteringColor(FLinearColor Value);
	
	UFUNCTION(BlueprintCallable, Category="Rendering|Components|ExponentialHeightFog")
	void SetFogHeightFalloff(float Value);
	
	UFUNCTION(BlueprintCallable, Category="Rendering|Components|ExponentialHeightFog")
	void SetFogMaxOpacity(float Value);
	
	UFUNCTION(BlueprintCallable, Category="Rendering|Components|ExponentialHeightFog")
	void SetStartDistance(float Value);

protected:
	// Begin UActorComponent interface.
	virtual void CreateRenderState_Concurrent() OVERRIDE;
	virtual void SendRenderTransform_Concurrent() OVERRIDE;
	virtual void DestroyRenderState_Concurrent() OVERRIDE;
	// End UActorComponent interface.

public:
	// Begin UObject Interface
	virtual void PostLoad() OVERRIDE; 
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	virtual void PostInterpChange(UProperty* PropertyThatChanged) OVERRIDE;
	// End UObject Interface
};



