// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	EngineMessagesModule.cpp: Implements the FEngineMessagesModule class.
=============================================================================*/

#include "EngineMessagesPrivatePCH.h"
#include "EngineMessages.generated.inl"


/**
 * Implements the EngineMessages module.
 */
class FEngineMessagesModule
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
UEngineServiceMessages::UEngineServiceMessages( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{ }


IMPLEMENT_MODULE(FEngineMessagesModule, EngineMessages);
