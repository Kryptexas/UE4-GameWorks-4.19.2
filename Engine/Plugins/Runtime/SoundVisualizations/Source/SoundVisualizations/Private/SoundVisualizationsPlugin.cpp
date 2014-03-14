// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SoundVisualizationsPrivatePCH.h"
#include "SoundVisualizations.generated.inl"



class FSoundVisualizationsPlugin : public ISoundVisualizationsPlugin
{
public:
	// IModuleInterface implementation
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;
	// End of IModuleInterface implementation
};

IMPLEMENT_MODULE( FSoundVisualizationsPlugin, SoundVisualizations )



void FSoundVisualizationsPlugin::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
}


void FSoundVisualizationsPlugin::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}



