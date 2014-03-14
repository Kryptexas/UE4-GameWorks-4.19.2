// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 *
 */

#include "BlueprintGeneratedClassFactory.generated.h"

UCLASS(hidecategories=Object, collapsecategories)
class UBlueprintGeneratedClassFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	// The parent class of the created blueprint
	UPROPERTY(EditAnywhere, Category=BlueprintFactory, meta=(AllowAbstract = "", BlueprintBaseOnly = ""))
	TSubclassOf<class UObject> ParentClass;

	// Begin UFactory Interface
	virtual bool ConfigureProperties() OVERRIDE;
	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) OVERRIDE;
	// Begin UFactory Interface

protected:
	// Hook for derived classes to indicate if they want to skip some filtering that is unnecessary for macro libraries
	virtual bool IsMacroFactory() const { return false; }
};



