// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"

class FEditorUtilityInstanceDetails : public IDetailCustomization
{
public:
	// Makes a new instance of this detail layout class for a specific detail view requesting it
	static TSharedRef<IDetailCustomization> MakeInstance();

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) OVERRIDE;
	// End of IDetailCustomization interface

protected:
	FReply OnExecuteAction(TWeakObjectPtr<UFunction> WeakFunctionPtr);

private:
	TArray< TWeakObjectPtr<UObject> > SelectedObjectsList;
};
