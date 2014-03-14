// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AndroidPlatformEditorPrivatePCH.h"
#include "AndroidTargetSettingsCustomization.h"
#include "Settings.h"

#define LOCTEXT_NAMESPACE "FAndroidPlatformEditorModule"

/**
 * Module for Android platform editor utilities
 */
class FAndroidPlatformEditorModule : public IModuleInterface
{
	// IModuleInterface interface
	virtual void StartupModule() OVERRIDE
	{
		// Register the settings detail panel customization
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomPropertyLayout(
			UAndroidRuntimeSettings::StaticClass(),
			FOnGetDetailCustomizationInstance::CreateStatic(&FAndroidTargetSettingsCustomization::MakeInstance));
		PropertyModule.NotifyCustomizationModuleChanged();

		ISettingsModule* SettingsModule = ISettingsModule::Get();

		if (SettingsModule != nullptr)
		{
			SettingsModule->RegisterSettings("Project", "Platforms", "Android",
				LOCTEXT("RuntimeSettingsName", "Android"),
				LOCTEXT("RuntimeSettingsDescription", "Settings and resources for Android platforms"),
				TWeakObjectPtr<UObject>(GetMutableDefault<UAndroidRuntimeSettings>())
				);
		}
	}

	virtual void ShutdownModule() OVERRIDE
	{
		ISettingsModule* SettingsModule = ISettingsModule::Get();

		if (SettingsModule != nullptr)
		{
			SettingsModule->UnregisterSettings("Project", "Platforms", "Android");
		}
	}
	// End of IModuleInterface interface
};

IMPLEMENT_MODULE(FAndroidPlatformEditorModule, AndroidPlatformEditor);

#undef LOCTEXT_NAMESPACE
