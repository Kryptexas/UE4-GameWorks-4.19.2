// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/**
 * Factory for ProceduralFoliage assets
 */

#pragma once

#include "ProceduralFoliageFactory.generated.h"


UCLASS()
class UProceduralFoliageFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	// UFactory interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual uint32 GetMenuCategories() const;
	// End of UFactory interface
};
