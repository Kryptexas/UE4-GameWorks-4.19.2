// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AppFrameworkPrivatePCH.h"


static const FName AppFrameworkTabName("AppFramework");


/**
 * Implements the AppFramework module.
 */
class FAppFrameworkModule
	: public IModuleInterface
{
public:

	// IModuleInterface interface
	
	virtual void StartupModule( ) override
	{
	}

	virtual void ShutdownModule( ) override
	{
	}
};


IMPLEMENT_MODULE(FAppFrameworkModule, AppFramework);
