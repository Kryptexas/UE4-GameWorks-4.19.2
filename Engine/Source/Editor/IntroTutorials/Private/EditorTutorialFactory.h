// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EditorTutorialFactory.generated.h"

class SEditorTutorialImporter;

UCLASS(hidecategories=Object, collapsecategories)
class UEditorTutorialFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	// UFactory Interface
	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual uint32 GetMenuCategories() const;
	virtual FText GetDisplayName() const;
	// End of UFactory Interface

private:
	/** Importer dialog reference */
	TSharedPtr<SEditorTutorialImporter> ImporterDialog;
};
