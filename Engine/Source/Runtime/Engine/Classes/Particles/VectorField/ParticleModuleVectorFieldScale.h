// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
	ParticleModuleVectorFieldScale: Per-particle vector field scale.
==============================================================================*/

#pragma once

#include "ParticleModuleVectorFieldScale.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, MinimalAPI, meta=(DisplayName = "Vector Field Scale"))
class UParticleModuleVectorFieldScale : public UParticleModuleVectorFieldBase
{
	GENERATED_UCLASS_BODY()

	/** Per-particle vector field scale. Evaluated using emitter time. */
	UPROPERTY(EditAnywhere, Category=VectorField)
	class UDistributionFloat* VectorFieldScale;

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
	friend class FParticleModuleVectorFieldScaleDetails;
};



