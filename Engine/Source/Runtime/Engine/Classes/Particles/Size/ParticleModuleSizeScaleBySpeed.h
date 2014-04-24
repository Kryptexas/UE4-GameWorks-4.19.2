// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
	ParticleModuleSizeScaleBySpeed: Scale the size of a particle by its velocity.
==============================================================================*/

#pragma once

#include "ParticleModuleSizeScaleBySpeed.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, meta=(DisplayName = "Size By Speed"))
class UParticleModuleSizeScaleBySpeed : public UParticleModuleSizeBase
{
	GENERATED_UCLASS_BODY()

	/** By how much speed affects the size of the particle in each dimension. */
	UPROPERTY(EditAnywhere, Category=ParticleModuleSizeScaleBySpeed)
	FVector2D SpeedScale;

	/** The maximum amount by which to scale a particle in each dimension. */
	UPROPERTY(EditAnywhere, Category=ParticleModuleSizeScaleBySpeed)
	FVector2D MaxScale;


	// Begin UParticleModule Interface
	virtual void CompileModule( struct FParticleEmitterBuildInfo& EmitterInfo ) OVERRIDE;
	// End UParticleModule Interface
};



