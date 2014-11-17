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
	virtual void StartupModule() override
	{
		// Register the settings detail panel customization
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomClassLayout(
			"IOSRuntimeSettings",
			FOnGetDetailCustomizationInstance::CreateStatic(&FIOSTargetSettingsCustomization::MakeInstance));
		PropertyModule.NotifyCustomizationModuleChanged();

		ISettingsModule* SettingsModule = ISettingsModule::Get();

		if (SettingsModule != nullptr)
		{
			SettingsModule->RegisterSettings("Project", "Platforms", "iOS",
				LOCTEXT("RuntimeSettingsName", "iOS"),
				LOCTEXT("RuntimeSettingsDescription", "Settings and resources for the iOS platform"),
				GetMutableDefault<UIOSRuntimeSettings>()
			);
		}
	}

	virtual void ShutdownModule() override
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
