
// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	InputBindingEditorModule.cpp: Implements the FInputBindingEditorModule class.
=============================================================================*/

#include "InputBindingEditorPrivatePCH.h"


/**
 * Implements the LauncherUI module.
 */
class FInputBindingEditorModule
	: public IInputBindingEditorModule
{
public:

	// Begin IInputBindingEditorModule interface

	virtual TWeakPtr<SWidget> CreateInputBindingEditorPanel( ) OVERRIDE
	{
		TSharedPtr<SWidget> Panel = SNew(SInputBindingEditorPanel);
		BindingEditorPanels.Add(Panel);

		return Panel;
	}

	virtual void DestroyInputBindingEditorPanel( const TWeakPtr<SWidget>& Panel ) OVERRIDE
	{
		BindingEditorPanels.Remove(Panel.Pin());
	}

	// End IInputBindingEditorModule interface


private:

	// Holds the collection of created binding editor panels.
	TArray<TSharedPtr<SWidget> > BindingEditorPanels;
};


IMPLEMENT_MODULE(FInputBindingEditorModule, InputBindingEditor);
