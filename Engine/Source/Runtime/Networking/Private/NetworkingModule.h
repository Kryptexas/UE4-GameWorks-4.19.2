// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	NetworkingModule.h: Declares the FNetworkingModule class.
=============================================================================*/

#pragma once


/**
 * Implements the Networking module.
 */
class FNetworkingModule
	: public INetworkingModule
{
public:

	virtual void StartupModule( ) override;

	virtual void ShutdownModule( ) override;

	virtual bool SupportsDynamicReloading( ) override;
};