// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	EngineMessagesModule.cpp: Implements the FEngineMessagesModule class.
=============================================================================*/

#include "LaunchDaemonMessagesPrivatePCH.h"
#include "LaunchDaemonMessages.generated.inl"


/**
 * Implements the EngineMessages module.
 */
class FLaunchDaemonMessagesModule
	: public IModuleInterface
{
public:

	virtual void StartupModule( ) OVERRIDE { }

	virtual void ShutdownModule( ) OVERRIDE { }

	virtual bool SupportsDynamicReloading( ) OVERRIDE
	{
		return true;
	}
};

// Dummy class initialization
UIOSMessageProtocol::UIOSMessageProtocol( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{ }

IMPLEMENT_MODULE(FLaunchDaemonMessagesModule, LaunchDaemonMessages);
