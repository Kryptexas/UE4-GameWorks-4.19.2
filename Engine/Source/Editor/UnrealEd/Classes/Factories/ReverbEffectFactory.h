// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// ReverbEffectFactory
//=============================================================================

#pragma once
#include "ReverbEffectFactory.generated.h"

UCLASS(hidecategories=Object)
class UReverbEffectFactory : public UFactory
{
	GENERATED_BODY()
public:
	UReverbEffectFactory(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) override;
	// Begin UFactory Interface	
};



