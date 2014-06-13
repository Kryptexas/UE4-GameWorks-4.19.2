// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Implements the game launcher.
 */
class FLauncher
	: public ILauncher
{
public:

	// Begin ILauncher interface

	virtual ILauncherWorkerPtr Launch( const ITargetDeviceProxyManagerRef& DeviceProxyManager, const ILauncherProfileRef& Profile ) OVERRIDE;

	// End ILauncher interface
};
