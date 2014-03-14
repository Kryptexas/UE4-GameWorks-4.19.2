// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 *
 */

#pragma once
#include "BlueprintMacroFactory.generated.h"

UCLASS(hidecategories=Object, collapsecategories)
class UBlueprintMacroFactory : public UBlueprintFactory
{
	GENERATED_UCLASS_BODY()

	// Begin UFactory Interface
	virtual FText GetDisplayName() const OVERRIDE;
	virtual FName GetNewAssetThumbnailOverride() const OVERRIDE;
	virtual uint32 GetMenuCategories() const OVERRIDE;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) OVERRIDE;
	// Begin UFactory Interface

protected:
	// UBlueprintFactory interface
	virtual bool IsMacroFactory() const OVERRIDE { return true; }
	// End of UBlueprintFactory interface
};



