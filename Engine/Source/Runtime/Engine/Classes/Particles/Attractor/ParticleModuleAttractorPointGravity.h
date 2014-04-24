// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
	ParticleModuleAttractorPointGravity: Causes particles to accelerate towards
		a point in space.
==============================================================================*/

#pragma once

#include "ParticleModuleAttractorPointGravity.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, meta=(DisplayName = "Point Gravity"))
class UParticleModuleAttractorPointGravity : public UParticleModuleAttractorBase
{
	GENERATED_UCLASS_BODY()

	/** The position of the point gravity source. */
	UPROPERTY(EditAnywhere, Category=PointGravitySource)
	FVector Position;

	/** The distance at which the influence of the point begins to falloff. */
	UPROPERTY(EditAnywhere, Category=PointGravitySource)
	float Radius;

	/** The strength of the point source. */
	UPROPERTY(EditAnywhere, noclear, Category=PointGravitySource)
	class UDistributionFloat* Strength;

	/** Initializes the default values for this property */
	void InitializeDefaults();

	//Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	virtual void PostInitProperties() OVERRIDE;
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	//End UObject Interface

	//Begin UParticleModule Interface
	virtual void CompileModule(struct FParticleEmitterBuildInfo& EmitterInfo) OVERRIDE;
	virtual void Render3DPreview(FParticleEmitterInstance* Owner, const FSceneView* View,FPrimitiveDrawInterface* PDI) OVERRIDE;
	//End UParticleModule Interface
};



