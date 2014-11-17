// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ProfilerMessagesModule.cpp: Implements the FProfilerMessagesModule class.
=============================================================================*/

#include "ProfilerMessagesPrivatePCH.h"



/**
 * Implements the ProfilerMessages module.
 */
class FProfilerMessagesModule
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
UProfilerServiceMessages::UProfilerServiceMessages( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{ }


IMPLEMENT_MODULE(FProfilerMessagesModule, ProfilerMessages);
