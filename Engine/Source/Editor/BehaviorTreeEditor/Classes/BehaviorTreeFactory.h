// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTreeFactory.generated.h"

UCLASS(hidecategories=Object)
class UBehaviorTreeFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	// Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) OVERRIDE;
	virtual bool ShouldShowInNewMenu() const OVERRIDE;
	// Begin UFactory Interface	
};



