// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEdMessagesPrivatePCH.h"


/**
 * Implements the UnrealEdMessages module.
 */
class FUnrealEdMessagesModule
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

// Dummy class initialization
UAssetEditorMessages::UAssetEditorMessages( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{ }

IMPLEMENT_MODULE(FUnrealEdMessagesModule, UnrealEdMessages);
