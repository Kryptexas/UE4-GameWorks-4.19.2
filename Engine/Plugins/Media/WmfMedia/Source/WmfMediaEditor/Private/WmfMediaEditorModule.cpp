// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "WmfMediaEditorPrivatePCH.h"


/**
 * Implements the WmfMediaEditor module.
 */
class FWmfMediaEditorModule
	: public IModuleInterface
{
public:

	// IModuleInterface interface

	virtual void StartupModule( ) override { }

	virtual void ShutdownModule( ) override { }
	
	virtual bool SupportsDynamicReloading( ) override
	{
		return true;
	}
};


IMPLEMENT_MODULE(FWmfMediaEditorModule, WmfMediaEditor);
