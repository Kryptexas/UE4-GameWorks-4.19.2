// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SettingsEditorPrivatePCH.h"


/**
 * Implements the SettingsEditor module.
 */
class FSettingsEditorModule
	: public ISettingsEditorModule
{
public:

	// ISettingsEditorModule interface

	virtual TSharedRef<SWidget> CreateEditor( const ISettingsEditorModelRef& Model ) override
	{
		return SNew(SSettingsEditor, Model);
	}

	virtual ISettingsEditorModelRef CreateModel( const ISettingsContainerRef& SettingsContainer ) override
	{
		return MakeShareable(new FSettingsEditorModel(SettingsContainer));
	}
};


IMPLEMENT_MODULE(FSettingsEditorModule, SettingsEditor);
