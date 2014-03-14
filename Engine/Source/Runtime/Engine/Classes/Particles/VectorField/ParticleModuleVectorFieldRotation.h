// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
	ParticleModuleVectorFieldRotation: Random initial rotation for local
		vector fields.
==============================================================================*/

#pragma once

#include "ParticleModuleVectorFieldRotation.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, meta=(DisplayName = "VF Init Rotation"))
class UParticleModuleVectorFieldRotation : public UParticleModuleVectorFieldBase
{
	GENERATED_UCLASS_BODY()

	/** Minimum initial rotation applied to the local vector field. */
	UPROPERTY(EditAnywhere, Category=VectorField)
	FVector MinInitialRotation;

	/** Maximum initial rotation applied to the local vector field. */
	UPROPERTY(EditAnywhere, Category=VectorField)
	FVector MaxInitialRotation;


	// Begin UParticleModule Interface
	virtual void CompileModule(struct FParticleEmitterBuildInfo& EmitterInfo) OVERRIDE;
	// Begin UParticleModule Interface
};

