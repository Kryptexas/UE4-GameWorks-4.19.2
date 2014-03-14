// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IOSTargetPlatformPrivatePCH.h"

/**
 * Holds the target platform singleton.
 */
static ITargetPlatform* Singleton = NULL;


/**
 * Module for iOS as a target platform
 */
class FIOSTargetPlatformModule : public ITargetPlatformModule
{
public:

	/**
	 * Destructor.
	 */
	~FIOSTargetPlatformModule()
	{
		Singleton = NULL;
	}


public:

	// Begin ITargetPlatformModule interface

	virtual ITargetPlatform* GetTargetPlatform()
	{
		if (Singleton == NULL)
		{
			Singleton = new FIOSTargetPlatform();
		}

		return Singleton;
	}

	// End ITargetPlatformModule interface


public:

	// Begin IModuleInterface interface
	virtual void StartupModule() OVERRIDE
	{
	}

	virtual void ShutdownModule() OVERRIDE
	{
	}
	// End IModuleInterface interface
};

IMPLEMENT_MODULE( FIOSTargetPlatformModule, IOSTargetPlatform);
