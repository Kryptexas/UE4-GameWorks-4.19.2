// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// ParticleSystemFactoryNew
//=============================================================================

#pragma once
#include "ParticleSystemFactoryNew.generated.h"

UCLASS(hidecategories=Object)
class UParticleSystemFactoryNew : public UFactory
{
	GENERATED_BODY()
public:
	UParticleSystemFactoryNew(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) override;
	// Begin UFactory Interface	
};



