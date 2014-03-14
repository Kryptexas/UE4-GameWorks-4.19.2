// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Core.h"
#include "ModuleInterface.h"
#include "OnlineSubsystemIOSPackage.h"

/**
 * Online subsystem module class (GameCenter Implementation)
 * Code related to the loading and handling of the GameCenter module.
 */
class FOnlineSubsystemIOSModule : public IModuleInterface
{
private:

	virtual ~FOnlineSubsystemIOSModule(){}

	// Begin IModuleInterface interface
	virtual void StartupModule() OVERRIDE;

	virtual void ShutdownModule() OVERRIDE;

	virtual bool SupportsDynamicReloading() OVERRIDE
	{
		return false;
	}

	virtual bool SupportsAutomaticShutdown() OVERRIDE
	{
		return false;
	}
	// End IModuleInterface interface
};

typedef TSharedPtr<FOnlineSubsystemIOSModule, ESPMode::ThreadSafe> FOnlineSubsystemIOSModulePtr;

