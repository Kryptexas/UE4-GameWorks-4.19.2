// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ParticleModuleLocationWorldOffset.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, meta=(DisplayName = "World Offset"))
class UParticleModuleLocationWorldOffset : public UParticleModuleLocation
{
	GENERATED_UCLASS_BODY()


protected:
	//Begin UParticleModuleLocation Interface
	virtual void SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, class FRandomStream* InRandomStream, FBaseParticle* ParticleBase) OVERRIDE;
	//End UParticleModuleLocation Interface
};

