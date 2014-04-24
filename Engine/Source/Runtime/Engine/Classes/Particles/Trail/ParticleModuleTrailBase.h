// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 *	ParticleModuleTrailBase
 *	Provides the base class for Trail emitter modules
 *
 */

#pragma once
#include "ParticleModuleTrailBase.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, abstract, meta=(DisplayName = "Trail"))
class UParticleModuleTrailBase : public UParticleModule
{
	GENERATED_UCLASS_BODY()


	// Begin UParticleModule Interface
	virtual EModuleType	GetModuleType() const OVERRIDE {	return EPMT_Trail;	}
	virtual bool CanTickInAnyThread() OVERRIDE
	{
		return false;
	}
	// End UParticleModule Interface
};



