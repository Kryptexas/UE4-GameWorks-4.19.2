// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ILauncherServicesModule.h: Declares the ILauncherServicesModule interface.
=============================================================================*/

#include "LauncherAutomatedServicePrivatePCH.h"

#include "ModuleManager.h"

IMPLEMENT_MODULE(FLauncherAutomatedServiceModule, LauncherAutomatedService);

ILauncherAutomatedServiceProviderPtr FLauncherAutomatedServiceModule::CreateAutomatedServiceProvider()
{
	return MakeShareable(new FLauncherAutomatedServiceProvider());
}