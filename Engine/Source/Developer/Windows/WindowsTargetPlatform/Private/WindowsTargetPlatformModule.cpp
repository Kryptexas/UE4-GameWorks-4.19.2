// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	WindowsTargetPlatformModule.cpp: Implements the FWindowsTargetPlatformModule class.
=============================================================================*/

#include "WindowsTargetPlatformPrivatePCH.h"


#define LOCTEXT_NAMESPACE "FWindowsTargetPlatformModule"


/**
 * Holds the target platform singleton.
 */
static ITargetPlatform* Singleton = NULL;


/**
 * Implements the Windows target platform module.
 */
class FWindowsTargetPlatformModule
	: public ITargetPlatformModule
{
public:

	/**
	 * Destructor.
	 */
	~FWindowsTargetPlatformModule( )
	{
		Singleton = NULL;
	}


public:

	// Begin ITargetPlatformModule interface

	virtual ITargetPlatform* GetTargetPlatform( ) OVERRIDE
	{
		if (Singleton == NULL)
		{
			Singleton = new TGenericWindowsTargetPlatform<true, false, false>();
		}

		return Singleton;
	}

	// End ITargetPlatformModule interface


public:

	// this is an example of a hotfix, declared here for no particular reason. Once we have other examples, it can be deleted.
#if 0
	void HotfixTest(void *InPayload, int PayloadSize)
	{
		check(sizeof(FTestHotFixPayload) == PayloadSize);
		
		FTestHotFixPayload* Payload = (FTestHotFixPayload*)InPayload;
		UE_LOG(LogTemp, Log, TEXT("Hotfix Test %s"), *Payload->Message);
		Payload->Result = Payload->ValueToReturn;
	}
#endif
	// Begin IModuleInterface interface

	virtual void StartupModule() OVERRIDE
	{
		// this is an example of a hotfix, declared here for no particular reason. Once we have other examples, it can be deleted.
#if 0
		FCoreDelegates::GetHotfixDelegate(EHotfixDelegates::Test).BindRaw(this, &FWindowsTargetPlatformModule::HotfixTest);
#endif

		TargetSettings = ConstructObject<UWindowsTargetSettings>(UWindowsTargetSettings::StaticClass(), GetTransientPackage(), "WindowsTargetSettings", RF_Standalone);
		TargetSettings->AddToRoot();

		ISettingsModule* SettingsModule = ISettingsModule::Get();

		if (SettingsModule != nullptr)
		{
			SettingsModule->RegisterSettings("Project", "Platforms", "Windows",
				LOCTEXT("TargetSettingsName", "Windows"),
				LOCTEXT("TargetSettingsDescription", "Settings for Windows target platform"),
				TargetSettings
			);
		}

#if WITH_EDITOR
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomPropertyLayout( TEXT("WindowsTargetSettings"), FOnGetDetailCustomizationInstance::CreateStatic( &FWindowsTargetSettingsDetails::MakeInstance ) );
#endif
	}

	virtual void ShutdownModule() OVERRIDE
	{
		// this is an example of a hotfix, declared here for no particular reason. Once we have other examples, it can be deleted.
#if 0
		FCoreDelegates::GetHotfixDelegate(EHotfixDelegates::Test).Unbind();
#endif

		ISettingsModule* SettingsModule = ISettingsModule::Get();

		if (SettingsModule != nullptr)
		{
			SettingsModule->UnregisterSettings("Project", "Platforms", "Windows");
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

#if WITH_EDITOR
		if(FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
		{
			FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
			PropertyModule.UnregisterCustomPropertyLayout( TEXT("WindowsTargetSettings") );
		}
#endif
	}

	// End IModuleInterface interface


private:

	// Holds the target settings.
	UWindowsTargetSettings* TargetSettings;
};


#undef LOCTEXT_NAMESPACE


IMPLEMENT_MODULE(FWindowsTargetPlatformModule, WindowsTargetPlatform);
