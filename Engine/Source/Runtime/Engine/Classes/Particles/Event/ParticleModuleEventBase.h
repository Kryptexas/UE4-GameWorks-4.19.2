// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "Particles/ParticleModule.h"
#include "ParticleModuleEventBase.generated.h"

UCLASS(editinlinenew, hidecategories=Object, abstract, meta=(DisplayName = "Event"))
class UParticleModuleEventBase : public UParticleModule
{
	GENERATED_BODY()
public:
	UParticleModuleEventBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// Begin UParticleModule Interface
	virtual EModuleType	GetModuleType() const override {	return EPMT_Event;	}
	// End UParticleModule Interface
};

