// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ParticleModuleMeshRotationRateOverLife.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, meta=(DisplayName = "Mesh Rotation Rate over Life"))
class UParticleModuleMeshRotationRateOverLife : public UParticleModuleRotationRateBase
{
	GENERATED_UCLASS_BODY()

	/**
	 *	The rotation rate desired.
	 *	The value is retrieved using the RelativeTime of the particle.
	 */
	UPROPERTY(EditAnywhere, Category=Rotation)
	struct FRawDistributionVector RotRate;

	/**
	 *	If true, scale the current rotation rate by the value retrieved.
	 *	Otherwise, set the rotation rate to the value retrieved.
	 */
	UPROPERTY(EditAnywhere, Category=Rotation)
	uint32 bScaleRotRate:1;

	//Begin UObject Interface
	virtual void	PostInitProperties() OVERRIDE;
	//End UObject Interface

	// Begin UParticleModule Interface
	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) OVERRIDE;
	virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) OVERRIDE;
	virtual void SetToSensibleDefaults(UParticleEmitter* Owner) OVERRIDE;
	virtual bool TouchesMeshRotation() const OVERRIDE { return true; }
	// End UParticleModule Interface
};



