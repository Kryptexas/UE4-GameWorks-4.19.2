// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ProfilerServiceModule.cpp: Implements the FProfilerServiceModule class.
=============================================================================*/

#include "ProfilerServicePrivatePCH.h"

#include "ModuleManager.h"

/**
 * Implements the ProfilerService module
 */
class FProfilerServiceModule
	: public IProfilerServiceModule
{
public:

	// Begin IModuleInterface interface
	virtual void ShutdownModule()
	{
		if (ProfilerServiceManager.IsValid())
		{
			((FProfilerServiceManager*)ProfilerServiceManager.Get())->Shutdown();
		}
		ProfilerServiceManager.Reset();
	}
	// End IModuleInterface interface

	// Begin IProfilerServiceModule interface
	virtual IProfilerServiceManagerPtr CreateProfilerServiceManager( ) OVERRIDE
	{
		if (!ProfilerServiceManager.IsValid())
		{
			ProfilerServiceManager = FProfilerServiceManager::CreateSharedServiceManager();
			((FProfilerServiceManager*)ProfilerServiceManager.Get())->Init();
		}

		return ProfilerServiceManager;
	}
	// End IProfilerServiceModule interface

private:
	/**
	 * Holds the profiler service singleton
	 */
	static IProfilerServiceManagerPtr ProfilerServiceManager;
};

/* Static initialization
******************************************************************************/
IProfilerServiceManagerPtr FProfilerServiceModule::ProfilerServiceManager = NULL;


IMPLEMENT_MODULE(FProfilerServiceModule, ProfilerService);
