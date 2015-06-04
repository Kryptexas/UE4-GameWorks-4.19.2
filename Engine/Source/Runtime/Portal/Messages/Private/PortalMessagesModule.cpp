// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "PortalMessagesPrivatePCH.h"
#include "ModuleInterface.h"


/**
 * Implements the module.
 */
class FModule
	: public IModuleInterface
{
public:

	// IModuleInterface interface

	virtual void StartupModule() override { }
	virtual void ShutdownModule() override { }

	virtual bool SupportsDynamicReloading() override
	{
		return true;
	}
};


IMPLEMENT_MODULE(FModule, PortalMessages);
