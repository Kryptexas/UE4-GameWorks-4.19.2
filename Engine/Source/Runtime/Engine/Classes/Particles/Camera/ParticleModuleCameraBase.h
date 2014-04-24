// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ParticleModuleCameraBase.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, abstract, meta=(DisplayName = "Camera"))
class UParticleModuleCameraBase : public UParticleModule
{
	GENERATED_UCLASS_BODY()

	virtual bool CanTickInAnyThread() OVERRIDE
	{
		return false;
	}
};

