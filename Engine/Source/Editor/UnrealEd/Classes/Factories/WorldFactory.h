// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

/**
 *
 */

#include "WorldFactory.generated.h"

UCLASS(MinimalAPI)
class UWorldFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	TEnumAsByte<EWorldType::Type> WorldType;

	// Begin UFactory Interface
	virtual bool ConfigureProperties() OVERRIDE;
	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) OVERRIDE;
	// Begin UFactory Interface	
};

