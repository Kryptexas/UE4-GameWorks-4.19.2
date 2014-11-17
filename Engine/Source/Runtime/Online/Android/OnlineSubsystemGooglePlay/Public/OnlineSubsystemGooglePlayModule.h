// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Core.h"
#include "ModuleInterface.h"

/**
 * Online subsystem module class (Google Play Implementation)
 * Code related to the loading and handling of the Amdroid Google Play module.
 */
class FOnlineSubsystemGooglePlayModule : public IModuleInterface
{
private:
	
	/** Class responsible for creating instance(s) of the subsystem */
	class FOnlineFactoryGooglePlay* GooglePlayFactory;

	virtual ~FOnlineSubsystemGooglePlayModule(){}

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

typedef TSharedPtr<FOnlineSubsystemGooglePlayModule> FOnlineSubsystemGooglePlayModulePtr;

