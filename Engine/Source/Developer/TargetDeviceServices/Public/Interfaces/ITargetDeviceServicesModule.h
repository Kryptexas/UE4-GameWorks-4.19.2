// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ITargetDeviceServicesModule.h: Declares the ITargetDeviceServicesModule interface.
=============================================================================*/

#pragma once


/**
 * Interface for target device services modules.
 */
class ITargetDeviceServicesModule
	: public IModuleInterface
{
public:

	/**
	 * Gets the target device proxy manager.
	 *
	 * @return The device proxy manager.
	 */
	virtual ITargetDeviceProxyManagerRef GetDeviceProxyManager( ) = 0;

	/**
	 * Gets the target device service manager.
	 *
	 * @return The device service manager.
	 */
	virtual ITargetDeviceServiceManagerRef GetDeviceServiceManager( ) = 0;

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~ITargetDeviceServicesModule( ) { }
};
