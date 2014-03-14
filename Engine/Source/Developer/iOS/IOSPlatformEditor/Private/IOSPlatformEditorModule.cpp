// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IOSPlatformEditorPrivatePCH.h"
#include "IOSTargetSettingsCustomization.h"
#include "Settings.h"

#define LOCTEXT_NAMESPACE "FIOSPlatformEditorModule"

/**
 * Module for iOS as a target platform
 */
class FIOSPlatformEditorModule : public IModuleInterface
{
	// IModuleInterface interface
	virtual void StartupModule() OVERRIDE
	{
		// Register the settings detail panel customization
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomPropertyLayout(
			UIOSRuntimeSettings::StaticClass(),
			FOnGetDetailCustomizationInstance::CreateStatic(&FIOSTargetSettingsCustomization::MakeInstance));
		PropertyModule.NotifyCustomizationModuleChanged();

		ISettingsModule* SettingsModule = ISettingsModule::Get();

		if (SettingsModule != nullptr)
		{
			SettingsModule->RegisterSettings("Project", "Platforms", "iOS",
				LOCTEXT("RuntimeSettingsName", "iOS"),
				LOCTEXT("RuntimeSettingsDescription", "Settings and resources for the iOS platform"),
				TWeakObjectPtr<UObject>(GetMutableDefault<UIOSRuntimeSettings>())
				);
		}
	}

	virtual void ShutdownModule() OVERRIDE
	{
		ISettingsModule* SettingsModule = ISettingsModule::Get();

		if (SettingsModule != nullptr)
		{
			SettingsModule->UnregisterSettings("Project", "Platforms", "iOS");
		}
	}
	// End of IModuleInterface interface
};

IMPLEMENT_MODULE(FIOSPlatformEditorModule, IOSPlatformEditor);

#undef LOCTEXT_NAMESPACE
