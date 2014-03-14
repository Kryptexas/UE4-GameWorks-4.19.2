// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SettingsEditorModule.cpp: Implements the FSettingsEditorModule class.
=============================================================================*/

#include "SettingsEditorPrivatePCH.h"


/**
 * Implements the SettingsEditor module.
 */
class FSettingsEditorModule
	: public ISettingsEditorModule
{
public:

	// Begin ISettingsEditorModule interface

	virtual TSharedRef<SWidget> CreateEditor( const ISettingsEditorModelRef& Model ) OVERRIDE
	{
		return SNew(SSettingsEditor, Model);
	}

	virtual ISettingsEditorModelRef CreateModel( const ISettingsContainerRef& SettingsContainer ) OVERRIDE
	{
		return MakeShareable(new FSettingsEditorModel(SettingsContainer));
	}

	// End ISettingsEditorModule interface
};


IMPLEMENT_MODULE(FSettingsEditorModule, SettingsEditor);
