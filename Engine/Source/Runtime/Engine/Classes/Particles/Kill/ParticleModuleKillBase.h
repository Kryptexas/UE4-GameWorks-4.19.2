// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "Particles/ParticleModule.h"
#include "ParticleModuleKillBase.generated.h"

UCLASS(editinlinenew, hidecategories=Object, abstract, meta=(DisplayName = "Kill"))
class UParticleModuleKillBase : public UParticleModule
{
	GENERATED_BODY()
public:
	UParticleModuleKillBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

};

