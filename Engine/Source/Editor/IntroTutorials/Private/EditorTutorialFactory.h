// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EditorTutorialFactory.generated.h"

UCLASS(hidecategories=Object, collapsecategories)
class UEditorTutorialFactory : public UFactory
{
	GENERATED_BODY()
public:
	UEditorTutorialFactory(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual uint32 GetMenuCategories() const;
	virtual FText GetDisplayName() const;
	// End of UFactory Interface
};
