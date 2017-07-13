// Copyright 2017 Google Inc.

#pragma once

#include "Factories/Factory.h"
#include "MeshReconstructionFactoryNew.generated.h"



/**
 * Implements a factory for UTangoMeshReconstruction objects.
 */
UCLASS(hidecategories=Object, MinimalAPI)
class UMeshReconstructionFactoryNew
	: public UFactory
{
	GENERATED_UCLASS_BODY()

public:

	// UFactory Interface
	virtual uint32 GetMenuCategories() const override;
	virtual UObject* FactoryCreateNew( UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn ) override;
	virtual bool ShouldShowInNewMenu() const override;
};
