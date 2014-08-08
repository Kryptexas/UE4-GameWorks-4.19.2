// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "StructureFactory.generated.h"

UCLASS(hidecategories=Object, MinimalAPI)
class UStructureFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	class UScriptStruct* Base;

	// Begin UFactory Interface
	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) override;
	// Begin UFactory Interface
};



