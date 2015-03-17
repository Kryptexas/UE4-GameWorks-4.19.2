// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// MaterialParameterCollectionFactoryNew.h
//=============================================================================

#pragma once
#include "MaterialParameterCollectionFactoryNew.generated.h"

UCLASS(hidecategories=Object, collapsecategories)
class UMaterialParameterCollectionFactoryNew : public UFactory
{
	GENERATED_BODY()
public:
	UMaterialParameterCollectionFactoryNew(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) override;
	// Begin UFactory Interface	
};



