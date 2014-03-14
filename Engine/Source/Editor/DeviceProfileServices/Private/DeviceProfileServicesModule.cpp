// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DeviceProfileServicesModule.cpp: Implements the FDeviceProfileServicesModule module.
=============================================================================*/

#include "DeviceProfileServicesPCH.h"

/**
 * Implements the DeviceProfileServices module.
 */
class FDeviceProfileServicesModule
	: public IDeviceProfileServicesModule
{
public:

		// Begin IDeviceProfileServicesModule interface
		virtual IDeviceProfileServicesUIManagerRef GetProfileServicesManager( ) OVERRIDE
		{
			if (!DeviceProfileServicesUIManagerSingleton.IsValid())
			{
				DeviceProfileServicesUIManagerSingleton = MakeShareable(new FDeviceProfileServicesUIManager());
			}

			return DeviceProfileServicesUIManagerSingleton.ToSharedRef();
		}
		// End IDeviceProfileServicesModule interface

protected:
		// Holds the session manager singleton.
		static IDeviceProfileServicesUIManagerPtr DeviceProfileServicesUIManagerSingleton;
};


/* Static initialization
 *****************************************************************************/

IDeviceProfileServicesUIManagerPtr FDeviceProfileServicesModule::DeviceProfileServicesUIManagerSingleton = NULL;

IMPLEMENT_MODULE(FDeviceProfileServicesModule, DeviceProfileServices);
