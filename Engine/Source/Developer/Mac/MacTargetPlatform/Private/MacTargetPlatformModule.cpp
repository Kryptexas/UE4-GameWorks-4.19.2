// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MacTargetPlatformModule.cpp: Implements the FMacTargetPlatformModule class.
=============================================================================*/

#if PLATFORM_MAC
	#define FVector FVectorWorkaround

	#ifdef __OBJC__
		#import <Cocoa/Cocoa.h>
	#endif

	#undef check
	#undef verify
	#undef FVector
#endif

#include "MacTargetPlatformPrivatePCH.h"


#define LOCTEXT_NAMESPACE "FMacTargetPlatformModule"


/**
 * Holds the target platform singleton.
 */
static ITargetPlatform* Singleton = NULL;


/**
 * Module for Mac as a target platform
 */
class FMacTargetPlatformModule
	: public ITargetPlatformModule
{
public:

	/**
	 * Destructor.
	 */
	~FMacTargetPlatformModule()
	{
		Singleton = NULL;
	}


public:

	// Begin ITargetPlatformModule interface

	virtual ITargetPlatform* GetTargetPlatform()
	{
		if (Singleton == NULL)
		{
			Singleton = new TGenericMacTargetPlatform<true, false>;
		}

		return Singleton;
	}

	// End ITargetPlatformModule interface


public:

	// Begin IModuleInterface interface

	virtual void StartupModule() OVERRIDE
	{
		TargetSettings = ConstructObject<UMacTargetSettings>(UMacTargetSettings::StaticClass(), GetTransientPackage(), "MacTargetSettings", RF_Standalone);
		TargetSettings->AddToRoot();

		ISettingsModule* SettingsModule = ISettingsModule::Get();

		if (SettingsModule != nullptr)
		{
			SettingsModule->RegisterSettings("Project", "Platforms", "Mac",
				LOCTEXT("TargetSettingsName", "Mac"),
				LOCTEXT("TargetSettingsDescription", "Mac platform settings description text here"),
				TargetSettings
			);
		}
	}

	virtual void ShutdownModule() OVERRIDE
	{
		ISettingsModule* SettingsModule = ISettingsModule::Get();

		if (SettingsModule != nullptr)
		{
			SettingsModule->UnregisterSettings("Project", "Platforms", "Mac");
		}

		if (!GExitPurge)
		{
			// If we're in exit purge, this object has already been destroyed
			TargetSettings->RemoveFromRoot();
		}
		else
		{
			TargetSettings = NULL;
		}
	}

	// End IModuleInterface interface


private:

	// Holds the target settings.
	UMacTargetSettings* TargetSettings;
};


#undef LOCTEXT_NAMESPACE


IMPLEMENT_MODULE( FMacTargetPlatformModule, MacTargetPlatform);
