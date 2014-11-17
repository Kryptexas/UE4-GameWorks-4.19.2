// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EngineMessagesPrivatePCH.h"


/**
 * Implements the EngineMessages module.
 */
class FEngineMessagesModule
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
UEngineServiceMessages::UEngineServiceMessages( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{ }


IMPLEMENT_MODULE(FEngineMessagesModule, EngineMessages);
