// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ParticleModuleMeshRotationRateMultiplyLife.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, meta=(DisplayName = "Mesh Rotation Rate * Life"))
class UParticleModuleMeshRotationRateMultiplyLife : public UParticleModuleRotationRateBase
{
	GENERATED_UCLASS_BODY()

	/**
	 *	The scale factor that should be applied to the rotation rate.
	 *	The value is retrieved using the RelativeTime of the particle.
	 */
	UPROPERTY(EditAnywhere, Category=Rotation)
	struct FRawDistributionVector LifeMultiplier;


	//Begin UObject Interface
	virtual void	PostInitProperties() OVERRIDE;
	virtual void	Serialize(FArchive& Ar) OVERRIDE;
	//End UObject Interface

	// Begin UParticleModule Interface
	virtual void	Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) OVERRIDE;
	virtual void	Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) OVERRIDE;
	virtual void SetToSensibleDefaults(UParticleEmitter* Owner) OVERRIDE;
	// End UParticleModule Interface
};



