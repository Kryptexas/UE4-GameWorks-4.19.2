// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SessionMessagesPrivatePCH.h"


/**
 * Implements the SessionMessages module.
 */
class FSessionMessagesModule
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
USessionServiceMessages::USessionServiceMessages( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{ }


IMPLEMENT_MODULE(FSessionMessagesModule, SessionMessages);
