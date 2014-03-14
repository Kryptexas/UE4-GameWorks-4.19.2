// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
	ParticleModuleVectorFieldScaleOverLife: Per-particle vector field scale over life.
==============================================================================*/

#pragma once

#include "ParticleModuleVectorFieldScaleOverLife.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, MinimalAPI, meta=(DisplayName = "VF Scale/Life"))
class UParticleModuleVectorFieldScaleOverLife : public UParticleModuleVectorFieldBase
{
	GENERATED_UCLASS_BODY()

	/** Per-particle vector field scale. Evaluated using particle relative time. */
	UPROPERTY(EditAnywhere, Category=VectorField)
	class UDistributionFloat* VectorFieldScaleOverLife;

	// Begin UObject Interface
	virtual void PostInitProperties() OVERRIDE;
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	// End UObject Interface

	// Begin UParticleModule Interface
	virtual void CompileModule(struct FParticleEmitterBuildInfo& EmitterInfo) OVERRIDE;

#if WITH_EDITOR
	virtual bool IsValidForLODLevel(UParticleLODLevel* LODLevel, FString& OutErrorString) OVERRIDE;
#endif
	// End UParticleModule Interface

protected:
	friend class FParticleModuleVectorFieldScaleOverLifeDetails;
};



