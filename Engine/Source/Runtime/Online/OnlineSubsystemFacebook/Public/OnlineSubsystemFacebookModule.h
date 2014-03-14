// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Core.h"
#include "ModuleInterface.h"
#include "OnlineSubsystemFacebookPackage.h"

/**
 * Online subsystem module class (Facebook Implementation)
 * Code related to the loading and handling of the Facebook module.
 */
class FOnlineSubsystemFacebookModule : public IModuleInterface
{
public:	

	// IModuleInterface 

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

	// FOnlineSubsystemFacebookModule

	virtual ~FOnlineSubsystemFacebookModule(){}
};

typedef TSharedPtr<FOnlineSubsystemFacebookModule, ESPMode::ThreadSafe> FOnlineSubsystemFacebookModulePtr;

