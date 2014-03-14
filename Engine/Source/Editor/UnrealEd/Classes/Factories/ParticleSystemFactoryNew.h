// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// ParticleSystemFactoryNew
//=============================================================================

#pragma once
#include "ParticleSystemFactoryNew.generated.h"

UCLASS(hidecategories=Object)
class UParticleSystemFactoryNew : public UFactory
{
	GENERATED_UCLASS_BODY()


	// Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) OVERRIDE;
	// Begin UFactory Interface	
};



