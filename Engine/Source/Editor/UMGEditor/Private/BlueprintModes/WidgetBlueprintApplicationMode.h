// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/////////////////////////////////////////////////////
// FWidgetBlueprintApplicationMode

class FWidgetBlueprintApplicationMode : public FBlueprintEditorApplicationMode
{
public:
	FWidgetBlueprintApplicationMode(TSharedPtr<class FWidgetBlueprintEditor> InWidgetEditor, FName InModeName);

	// FApplicationMode interface
	// End of FApplicationMode interface

protected:
	UWidgetBlueprint* GetBlueprint() const;
	
	TSharedPtr<class FWidgetBlueprintEditor> GetBlueprintEditor() const;

	TWeakPtr<class FWidgetBlueprintEditor> MyWidgetBlueprintEditor;

	// Set of spawnable tabs in the mode
	FWorkflowAllowedTabSet TabFactories;
};
