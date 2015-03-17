// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// MaterialFunctionFactoryNew
//=============================================================================

#pragma once
#include "MaterialFunctionFactoryNew.generated.h"

UCLASS(hidecategories=Object, collapsecategories)
class UMaterialFunctionFactoryNew : public UFactory
{
	GENERATED_BODY()
public:
	UMaterialFunctionFactoryNew(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) override;
	// Begin UFactory Interface	
};



