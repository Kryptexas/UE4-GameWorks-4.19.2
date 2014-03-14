// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ProfilerMessagesModule.cpp: Implements the FProfilerMessagesModule class.
=============================================================================*/

#include "ProfilerMessagesPrivatePCH.h"
#include "ProfilerMessages.generated.inl"


/**
 * Implements the ProfilerMessages module.
 */
class FProfilerMessagesModule
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
UProfilerServiceMessages::UProfilerServiceMessages( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{ }


IMPLEMENT_MODULE(FProfilerMessagesModule, ProfilerMessages);
