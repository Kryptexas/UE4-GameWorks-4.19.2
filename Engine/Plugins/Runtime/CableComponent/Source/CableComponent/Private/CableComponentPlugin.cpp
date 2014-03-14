// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CableComponentPluginPrivatePCH.h"

#include "CableComponent.generated.inl"


class FCableComponentPlugin : public IModuleInterface
{
	/** IModuleInterface implementation */
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;
};

IMPLEMENT_MODULE( FCableComponentPlugin, CableComponent )



void FCableComponentPlugin::StartupModule()
{
	
}


void FCableComponentPlugin::ShutdownModule()
{
	
}



