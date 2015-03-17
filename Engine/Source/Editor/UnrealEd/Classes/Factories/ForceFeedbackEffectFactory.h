// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "ForceFeedbackEffectFactory.generated.h"

UCLASS(hidecategories=Object)
class UForceFeedbackEffectFactory : public UFactory
{
	GENERATED_BODY()
public:
	UForceFeedbackEffectFactory(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) override;
	// Begin UFactory Interface	
};



