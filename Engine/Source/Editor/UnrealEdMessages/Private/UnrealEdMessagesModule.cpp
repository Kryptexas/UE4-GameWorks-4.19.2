// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UnrealEdMessagesModule.cpp: Implements the FUnrealEdMessagesModule class.
=============================================================================*/

#include "UnrealEdMessagesPrivatePCH.h"
#include "UnrealEdMessages.generated.inl"


/**
 * Implements the UnrealEdMessages module.
 */
class FUnrealEdMessagesModule
	: public IModuleInterface
{
public:

	virtual void StartupModule( ) OVERRIDE { }

	virtual void ShutdownModule( ) OVERRIDE { }

	virtual bool SupportsDynamicReloading( ) OVERRIDE
	{
		return true;
	}
};


// Dummy class initialization
UAssetEditorMessages::UAssetEditorMessages( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{ }


IMPLEMENT_MODULE(FUnrealEdMessagesModule, UnrealEdMessages);
