// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 *
 */

#pragma once
#include "BlueprintInterfaceFactory.generated.h"

UCLASS(hidecategories=Object, collapsecategories)
class UBlueprintInterfaceFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	// Begin UFactory Interface
	virtual FText GetDisplayName() const OVERRIDE;
	virtual FName GetNewAssetThumbnailOverride() const OVERRIDE;
	virtual uint32 GetMenuCategories() const OVERRIDE;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) OVERRIDE;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) OVERRIDE;
	// Begin UFactory Interface
};



