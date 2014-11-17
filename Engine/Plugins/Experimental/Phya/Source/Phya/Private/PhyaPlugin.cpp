// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PhyaPluginPrivatePCH.h"




class FPhyaPlugin : public IPhyaPlugin
{
	/** IModuleInterface implementation */
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;
};

IMPLEMENT_MODULE( FPhyaPlugin, Phya )



void FPhyaPlugin::StartupModule()
{
	
}


void FPhyaPlugin::ShutdownModule()
{
	
}



