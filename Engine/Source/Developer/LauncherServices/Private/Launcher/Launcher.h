// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Launcher.h: Declares the FLauncher class.
=============================================================================*/

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
