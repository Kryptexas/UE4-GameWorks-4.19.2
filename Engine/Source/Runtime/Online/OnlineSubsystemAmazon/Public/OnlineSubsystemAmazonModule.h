// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleInterface.h"
#include "ModuleManager.h"

/**
 * Module used for talking with an Amazon service via Http requests
 */
class FOnlineSubsystemAmazonModule :
	public IModuleInterface
{
public:

	/**
	 * Constructor
	 */
	FOnlineSubsystemAmazonModule()
	{}

	/**
	 * Destructor
	 */
	virtual ~FOnlineSubsystemAmazonModule() 
	{}

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
