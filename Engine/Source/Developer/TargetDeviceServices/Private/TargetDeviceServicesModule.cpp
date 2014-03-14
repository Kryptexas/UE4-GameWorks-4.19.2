// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	TargetDeviceServicesModule.cpp: Implements the FTargetDeviceServicesModule class.
=============================================================================*/

#include "TargetDeviceServicesPrivatePCH.h"
#include "TargetDeviceServices.generated.inl"


DEFINE_LOG_CATEGORY(TargetDeviceServicesLog);


/**
 * Implements the TargetDeviceServices module.
 */
class FTargetDeviceServicesModule
	: public ITargetDeviceServicesModule
{
public:

	// Begin ITargetDeviceServicesModule interface

	virtual ITargetDeviceProxyManagerRef GetDeviceProxyManager( ) OVERRIDE
	{
		if (!DeviceProxyManagerSingleton.IsValid())
		{
			DeviceProxyManagerSingleton = MakeShareable(new FTargetDeviceProxyManager());
		}

		return DeviceProxyManagerSingleton.ToSharedRef();
	}

	virtual ITargetDeviceServiceManagerRef GetDeviceServiceManager( ) OVERRIDE
	{
		if (!DeviceServiceManagerSingleton.IsValid())
		{
			DeviceServiceManagerSingleton = MakeShareable(new FTargetDeviceServiceManager());
		}

		return DeviceServiceManagerSingleton.ToSharedRef();
	}

	// End ITargetDeviceServicesModule interface

private:

	// Holds the device proxy manager singleton.
	ITargetDeviceProxyManagerPtr DeviceProxyManagerSingleton;

	// Holds the device service manager singleton.
	ITargetDeviceServiceManagerPtr DeviceServiceManagerSingleton;
};


// Dummy class initialization
UTargetDeviceServiceMessages::UTargetDeviceServiceMessages( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{ }


IMPLEMENT_MODULE(FTargetDeviceServicesModule, TargetDeviceServices);
