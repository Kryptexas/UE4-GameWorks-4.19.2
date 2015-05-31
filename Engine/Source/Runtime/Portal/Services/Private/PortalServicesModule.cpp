// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "PortalServicesPrivatePCH.h"
#include "ModuleManager.h"


/**
 * Implements the PortalServices module.
 */
class FPortalServicesModule
	: public IModuleInterface
{
public:

	/** Virtual destructor. */
	virtual ~FPortalServicesModule() { }

public:

	// IModuleInterface interface

	virtual void StartupModule() override { }
	virtual void ShutdownModule() override { }
	virtual bool SupportsDynamicReloading() override { return true; }
};


IMPLEMENT_MODULE(FPortalServicesModule, PortalServices);
