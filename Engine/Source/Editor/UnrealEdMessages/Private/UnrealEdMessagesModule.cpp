// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UnrealEdMessagesModule.cpp: Implements the FUnrealEdMessagesModule class.
=============================================================================*/

#include "UnrealEdMessagesPrivatePCH.h"



/**
 * Implements the UnrealEdMessages module.
 */
class FUnrealEdMessagesModule
	: public IModuleInterface
{
public:

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
