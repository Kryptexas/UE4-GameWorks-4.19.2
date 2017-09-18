// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// FlexFluidSurfaceFactory
//=============================================================================

#pragma once
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Factories/Factory.h"
#include "FlexFluidSurfaceFactory.generated.h"

UCLASS(hidecategories=Object)
class FLEXEDITOR_API UFlexFluidSurfaceFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	// Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) override;
	// Begin UFactory Interface	
};



