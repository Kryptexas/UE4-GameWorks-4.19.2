// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
	ParticleModuleVectorFieldRotationRate: Rotation rate for vector fields.
==============================================================================*/

#pragma once

#include "ParticleModuleVectorFieldRotationRate.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, meta=(DisplayName = "VF Rotation Rate"))
class UParticleModuleVectorFieldRotationRate : public UParticleModuleVectorFieldBase
{
	GENERATED_UCLASS_BODY()

	/** Constant rotation rate applied to the local vector field. */
	UPROPERTY(EditAnywhere, Category=VectorField)
	FVector RotationRate;


	// Begin UParticleModule Interface
	virtual void CompileModule(struct FParticleEmitterBuildInfo& EmitterInfo) OVERRIDE;
	// Begin UParticleModule Interface
};

