// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// SoundSurroundFactory
//=============================================================================

#pragma once
#include "SoundMixFactory.generated.h"

UCLASS(hidecategories=Object)
class USoundMixFactory : public UFactory
{
	GENERATED_BODY()
public:
	USoundMixFactory(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) override;
	// Begin UFactory Interface	
};



