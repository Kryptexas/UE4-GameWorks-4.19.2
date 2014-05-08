// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LinuxTargetPlatformModule.cpp: Implements the FAndroidTargetPlatformModule class.
=============================================================================*/

#include "LinuxTargetPlatformPrivatePCH.h"


#define LOCTEXT_NAMESPACE "FLinuxTargetPlatformModule"


/**
 * Holds the target platform singleton.
 */
static ITargetPlatform* Singleton = NULL;


/**
 * Module for the Android target platform.
 */
class FLinuxTargetPlatformModule
	: public ITargetPlatformModule
{
public:

	/**
	 * Destructor.
	 */
	~FLinuxTargetPlatformModule( )
	{
		Singleton = NULL;
	}


public:
	
	// Begin ITargetPlatformModule interface

	virtual ITargetPlatform* GetTargetPlatform( ) OVERRIDE
	{
		if (Singleton == NULL)
		{
			Singleton = new TLinuxTargetPlatform<true, false, false>();
		}
		
		return Singleton;
	}

	// End ITargetPlatformModule interface


public:

	// Begin IModuleInterface interface

	virtual void StartupModule() OVERRIDE
	{
		TargetSettings = ConstructObject<ULinuxTargetSettings>(ULinuxTargetSettings::StaticClass(), GetTransientPackage(), "LinuxTargetSettings", RF_Standalone);
		TargetSettings->AddToRoot();

		ISettingsModule* SettingsModule = ISettingsModule::Get();

		if (SettingsModule != nullptr)
		{
			SettingsModule->RegisterSettings("Project", "Platforms", "Linux",
				LOCTEXT("TargetSettingsName", "Linux"),
				LOCTEXT("TargetSettingsDescription", "Linux platform settings description text here"),
				TargetSettings
			);
		}
	}

	virtual void ShutdownModule() OVERRIDE
	{
		TargetSettings->RemoveFromRoot();
	}

	// End IModuleInterface interface


private:

	// Holds the target settings.
	ULinuxTargetSettings* TargetSettings;
};


#undef LOCTEXT_NAMESPACE


IMPLEMENT_MODULE( FLinuxTargetPlatformModule, LinuxTargetPlatform);
