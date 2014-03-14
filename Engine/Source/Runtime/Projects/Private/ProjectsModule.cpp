// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ProjectsModule.cpp: Implements the FProjectsModule class.
=============================================================================*/

#include "ProjectsPrivatePCH.h"


/**
 * Implements the Projects module.
 */
class FProjectsModule
	: public IModuleInterface
{
public:

	virtual void StartupModule( ) OVERRIDE { }

	virtual void ShutdownModule( ) OVERRIDE { }

	virtual bool SupportsDynamicReloading( ) OVERRIDE
	{
		return true;
	}
};


IMPLEMENT_MODULE(FProjectsModule, Projects);
