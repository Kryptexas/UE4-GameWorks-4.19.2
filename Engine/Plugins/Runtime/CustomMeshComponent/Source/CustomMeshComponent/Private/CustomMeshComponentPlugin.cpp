// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CustomMeshComponentPluginPrivatePCH.h"

#include "CustomMeshComponent.generated.inl"


class FCustomMeshComponentPlugin : public ICustomMeshComponentPlugin
{
	/** IModuleInterface implementation */
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;
};

IMPLEMENT_MODULE( FCustomMeshComponentPlugin, CustomMeshComponent )



void FCustomMeshComponentPlugin::StartupModule()
{
	
}


void FCustomMeshComponentPlugin::ShutdownModule()
{
	
}



