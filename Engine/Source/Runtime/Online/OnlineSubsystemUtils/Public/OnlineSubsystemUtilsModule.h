// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Core.h"
#include "ModuleInterface.h"

/**
 * Online subsystem module class
 * Wraps the loading of an online subsystem by name and allows new services to register themselves for use
 */
class FOnlineSubsystemUtilsModule : public IModuleInterface
{
public:

	FOnlineSubsystemUtilsModule() {}
	virtual ~FOnlineSubsystemUtilsModule() {}

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

/** Public references to the online subsystem module pointer should use this */
typedef TSharedPtr<FOnlineSubsystemUtilsModule, ESPMode::ThreadSafe> FOnlineSubsystemUtilsModulePtr;

