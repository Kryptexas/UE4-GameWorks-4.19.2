// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LauncherAutomatedServiceModule.h: Declares the FLauncherAutomatedServiceModule class.
=============================================================================*/

#pragma once



/**
 * Implements the launchers automated service.
 */
class FLauncherAutomatedServiceModule
	: public ILauncherAutomatedServiceModule
{
public:
	// Begin ILauncherAutomatedServiceModule interface

	virtual ILauncherAutomatedServiceProviderPtr CreateAutomatedServiceProvider() OVERRIDE;

	// End ILauncherAutomatedServiceModule interface

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~FLauncherAutomatedServiceModule( ) { }
};