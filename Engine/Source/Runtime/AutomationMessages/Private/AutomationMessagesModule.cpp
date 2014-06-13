// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AutomationMessagesPrivatePCH.h"


/**
 * Implements the AutomationMessages module.
 */
class FAutomationMessagesModule
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
UAutomationWorkerMessages::UAutomationWorkerMessages( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{ }


IMPLEMENT_MODULE(FAutomationMessagesModule, AutomationMessages);
