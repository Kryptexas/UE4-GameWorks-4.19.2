// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ParticleModuleLight.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, MinimalAPI, meta=(DisplayName = "Light"))
class UParticleModuleLight : public UParticleModuleLightBase
{
	GENERATED_UCLASS_BODY()

	/** Whether to use physically based inverse squared falloff from the light.  If unchecked, the LightExponent distribution will be used instead. */
	UPROPERTY(EditAnywhere, Category=Light)
	bool bUseInverseSquaredFalloff;

	/** 
	 * Whether lights from this module should affect translucency.
	 * Use with caution.  Modules enabling this should only make a few particle lights at most, and the smaller they are, the less they will cost.
	 */
	UPROPERTY(EditAnywhere, Category=Light)
	bool bAffectsTranslucency;

	/** 
	 * Will draw wireframe spheres to preview the light radius if enabled.
	 * Note: this is intended for previewing and the value will not be saved, it will always revert to disabled.
	 */
	UPROPERTY(EditAnywhere, Transient, Category=Light)
	bool bPreviewLightRadius;

	/** Fraction of particles in this emitter to create lights on. */
	UPROPERTY(EditAnywhere, Category=Light)
	float SpawnFraction;

	/** Scale that is applied to the particle's color to calculate the light's color, and can be setup as a curve over the particle's lifetime. */
	UPROPERTY(EditAnywhere, Category=Light)
	struct FRawDistributionVector ColorScaleOverLife;

	/** Brightness scale for the light, which can be setup as a curve over the particle's lifetime. */
	UPROPERTY(EditAnywhere, Category=Light)
	struct FRawDistributionFloat BrightnessOverLife;

	/** Scales the particle's radius, to calculate the light's radius. */
	UPROPERTY(EditAnywhere, Category=Light)
	struct FRawDistributionFloat RadiusScale;

	/** Provides the light's exponent when inverse squared falloff is disabled. */
	UPROPERTY(EditAnywhere, Category=Light)
	struct FRawDistributionFloat LightExponent;

	/** Initializes the default values for this property */
	void InitializeDefaults();

	// Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	virtual void PostInitProperties() OVERRIDE;
	// End UObject Interface


	//Begin UParticleModule Interface
	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) OVERRIDE;
	virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) OVERRIDE;
	virtual uint32 RequiredBytes(FParticleEmitterInstance* Owner = NULL) OVERRIDE;
	virtual void SetToSensibleDefaults(UParticleEmitter* Owner) OVERRIDE;
	virtual EModuleType	GetModuleType() const OVERRIDE { return EPMT_Light; }
	virtual void Render3DPreview(FParticleEmitterInstance* Owner, const FSceneView* View,FPrimitiveDrawInterface* PDI) OVERRIDE;
	//End UParticleModule Interface

	void SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, class FRandomStream* InRandomStream, FBaseParticle* ParticleBase);
};



