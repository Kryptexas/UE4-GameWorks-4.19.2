// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SessionMessagesModule.cpp: Implements the FSessionMessagesModule class.
=============================================================================*/

#include "SessionMessagesPrivatePCH.h"
#include "SessionMessages.generated.inl"


/**
 * Implements the SessionMessages module.
 */
class FSessionMessagesModule
	: public IModuleInterface
{
public:

	// Begin IModuleInterface interface

	virtual void StartupModule( ) OVERRIDE { }

	virtual void ShutdownModule( ) OVERRIDE { }

	virtual bool SupportsDynamicReloading( ) OVERRIDE
	{
		return true;
	}

	// End IModuleInterface interface
};


// Dummy class initialization
USessionServiceMessages::USessionServiceMessages( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{ }


IMPLEMENT_MODULE(FSessionMessagesModule, SessionMessages);
