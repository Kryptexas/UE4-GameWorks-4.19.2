// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IDeviceProfileServicesModule.h: Declares the IDeviceProfileServicesModule interface.
=============================================================================*/

#pragma once

/**
 * Device Profile Editor module
 */
class IDeviceProfileServicesModule 
	: public IModuleInterface
{

public:

	/**
	 * Gets the profile services manager.
	 *
	 * @return - the profile services manager.
	 */
	virtual IDeviceProfileServicesUIManagerRef GetProfileServicesManager( ) = 0;

	/**
	 * Virtual destructor.
	 */
	virtual ~IDeviceProfileServicesModule( ) { }
};

