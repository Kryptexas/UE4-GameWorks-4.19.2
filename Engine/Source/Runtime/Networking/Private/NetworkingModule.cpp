// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	NetworkingModule.cpp: Implements the FNetworkingModule class.
=============================================================================*/

#include "NetworkingPrivatePCH.h"

#include "ModuleManager.h"


IMPLEMENT_MODULE(FNetworkingModule, Networking);


/* IModuleInterface interface
 *****************************************************************************/

void FNetworkingModule::StartupModule( )
{
	FIPv4Endpoint::Initialize();
}


void FNetworkingModule::ShutdownModule( )
{
}


bool FNetworkingModule::SupportsDynamicReloading( )
{
	return false;
}