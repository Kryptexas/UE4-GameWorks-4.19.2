// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "PrivatePCH.h"
#include "ModuleManager.h"


/**
 * Implements the PortalChatProxies module.
 */
class FPortalChatProxiesModule
	: public IPortalChatProxiesModule
{
public:

	// IModuleInterface interface

	virtual void StartupModule() override
	{
	}

	virtual void ShutdownModule() override
	{
	}

	virtual bool SupportsDynamicReloading() override
	{
		return false;
	}
};


IMPLEMENT_MODULE(FPortalChatProxiesModule, PortalChatProxies);
