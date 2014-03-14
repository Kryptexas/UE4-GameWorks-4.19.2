// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LauncherServicesModule.cpp: Implements the FLauncherServicesModule class.
=============================================================================*/

#include "LauncherServicesPrivatePCH.h"


/**
 * Implements the LauncherServices module.
 */
class FLauncherServicesModule
	: public ILauncherServicesModule
{
public:

	// Begin ILauncherServicesModule interface

	virtual ILauncherDeviceGroupRef CreateDeviceGroup( ) OVERRIDE
	{
		return MakeShareable(new FLauncherDeviceGroup());
	}

	virtual ILauncherDeviceGroupRef CreateDeviceGroup(const FGuid& Guid, const FString& Name ) OVERRIDE
	{
		return MakeShareable(new FLauncherDeviceGroup(Guid, Name));
	}

	virtual ILauncherRef CreateLauncher( ) OVERRIDE
	{
		return MakeShareable(new FLauncher());
	}

	virtual ILauncherProfileRef CreateProfile( const FString& ProfileName ) OVERRIDE
	{
		return MakeShareable(new FLauncherProfile(ProfileName));
	}

	virtual ILauncherProfileManagerRef GetProfileManager( ) OVERRIDE
	{
		if (!ProfileManagerSingleton.IsValid())
		{
			ProfileManagerSingleton = MakeShareable(new FLauncherProfileManager());
		}

		return ProfileManagerSingleton.ToSharedRef();
	}

	// End ILauncherServicesModule interface


private:

	// Holds the launcher profile manager singleton
	static ILauncherProfileManagerPtr ProfileManagerSingleton;
};


/* Static initialization
 *****************************************************************************/

ILauncherProfileManagerPtr FLauncherServicesModule::ProfileManagerSingleton = NULL;


IMPLEMENT_MODULE(FLauncherServicesModule, LauncherServices);
