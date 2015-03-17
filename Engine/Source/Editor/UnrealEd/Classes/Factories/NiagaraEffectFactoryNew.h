// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NiagaraEffectFactoryNew.generated.h"

UCLASS(hidecategories = Object)
class UNiagaraEffectFactoryNew : public UFactory
{
	GENERATED_BODY()
public:
	UNiagaraEffectFactoryNew(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	// Begin UFactory Interface	
};



