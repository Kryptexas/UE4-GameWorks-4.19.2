// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Core.h"
#include "ModuleInterface.h"

/**
 * Online subsystem module class  (Null Implementation)
 * Code related to the loading of the Null module
 */
class FOnlineSubsystemNullModule : public IModuleInterface
{
private:

public:

	FOnlineSubsystemNullModule()
	{}

	virtual ~FOnlineSubsystemNullModule() {}

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
};

typedef TSharedPtr<FOnlineSubsystemNullModule, ESPMode::ThreadSafe> FOnlineSubsystemNullModulePtr;