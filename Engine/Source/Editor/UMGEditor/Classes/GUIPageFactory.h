// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GUIPageFactory.generated.h"

UCLASS()
class UGUIPageFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	// UFactory interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) OVERRIDE;
	virtual FText GetDisplayName() const OVERRIDE;
	// End of UFactory interface
};
