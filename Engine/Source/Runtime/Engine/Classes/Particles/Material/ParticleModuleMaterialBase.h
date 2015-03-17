// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "Particles/ParticleModule.h"
#include "ParticleModuleMaterialBase.generated.h"

UCLASS(editinlinenew, hidecategories=Object, abstract, meta=(DisplayName = "Material"))
class UParticleModuleMaterialBase : public UParticleModule
{
	GENERATED_BODY()
public:
	UParticleModuleMaterialBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool CanTickInAnyThread() override
	{
		return false;
	}
};

