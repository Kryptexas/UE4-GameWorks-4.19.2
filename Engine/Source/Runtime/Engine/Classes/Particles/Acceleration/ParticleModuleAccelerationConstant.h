// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
	ParticleModuleAccelerationConstant: Constant particle acceleration.
==============================================================================*/

#pragma once

#include "ParticleModuleAccelerationConstant.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=(Object, Acceleration), meta=(DisplayName = "Const Acceleration"))
class UParticleModuleAccelerationConstant : public UParticleModuleAccelerationBase
{
	GENERATED_UCLASS_BODY()

	/** Constant acceleration for particles in this system. */
	UPROPERTY(EditAnywhere, Category=ParticleModuleAccelerationConstant)
	FVector Acceleration;


	//Begin UParticleModule Interface
	virtual void CompileModule( struct FParticleEmitterBuildInfo& EmitterInfo ) OVERRIDE;
	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) OVERRIDE;
	virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) OVERRIDE;
	//End UParticleModule Interface
};



