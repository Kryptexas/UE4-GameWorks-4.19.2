// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DataTableFactory.generated.h"

UCLASS(hidecategories=Object, MinimalAPI)
class UDataTableFactory : public UFactory
{
	GENERATED_BODY()
public:
	UNREALED_API UDataTableFactory(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY()
	class UScriptStruct* Struct;

	// Begin UFactory Interface
	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	// Begin UFactory Interface
};



