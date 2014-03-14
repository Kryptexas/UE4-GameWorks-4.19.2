// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AutomationMessagesModule.cpp: Implements the FAutomationMessagesModule class.
=============================================================================*/

#include "AutomationMessagesPrivatePCH.h"
#include "AutomationMessages.generated.inl"


/**
 * Implements the AutomationMessages module.
 */
class FAutomationMessagesModule
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
UAutomationWorkerMessages::UAutomationWorkerMessages( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{ }


IMPLEMENT_MODULE(FAutomationMessagesModule, AutomationMessages);
