// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LaunchDaemonMessagesPrivatePCH.h"


/**
 * Implements the EngineMessages module.
 */
class FLaunchDaemonMessagesModule
	: public IModuleInterface
{
public:

	virtual void StartupModule( ) override { }

	virtual void ShutdownModule( ) override { }

	virtual bool SupportsDynamicReloading( ) override
	{
		return true;
	}
};

// Dummy class initialization
UIOSMessageProtocol::UIOSMessageProtocol( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{ }

IMPLEMENT_MODULE(FLaunchDaemonMessagesModule, LaunchDaemonMessages);
