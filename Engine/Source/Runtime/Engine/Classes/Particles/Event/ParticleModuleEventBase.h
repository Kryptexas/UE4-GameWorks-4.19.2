// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ParticleModuleEventBase.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, abstract, meta=(DisplayName = "Event"))
class UParticleModuleEventBase : public UParticleModule
{
	GENERATED_UCLASS_BODY()


	// Begin UParticleModule Interface
	virtual EModuleType	GetModuleType() const OVERRIDE {	return EPMT_Event;	}
	// End UParticleModule Interface
};

