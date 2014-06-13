// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Implements the launchers automated service.
 */
class FLauncherAutomatedServiceModule
	: public ILauncherAutomatedServiceModule
{
public:

	// ILauncherAutomatedServiceModule interface

	virtual ILauncherAutomatedServiceProviderPtr CreateAutomatedServiceProvider() OVERRIDE;

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~FLauncherAutomatedServiceModule( ) { }
};
