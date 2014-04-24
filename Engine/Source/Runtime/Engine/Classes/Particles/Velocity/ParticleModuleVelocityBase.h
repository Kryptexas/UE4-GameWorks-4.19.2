// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ParticleModuleVelocityBase.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, abstract, meta=(DisplayName = "Velocity"))
class UParticleModuleVelocityBase : public UParticleModule
{
	GENERATED_UCLASS_BODY()

	/**
	 *	If true, then treat the velocity as world-space defined.
	 *	NOTE: LocalSpace emitters that are moving will see strange results...
	 */
	UPROPERTY(EditAnywhere, Category=Velocity)
	uint32 bInWorldSpace:1;

	/** If true, then apply the particle system components scale to the velocity value. */
	UPROPERTY(EditAnywhere, Category=Velocity)
	uint32 bApplyOwnerScale:1;

};

