// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AndroidPlatformEditorPrivatePCH.h"
#include "AndroidTargetSettingsCustomization.h"
#include "AndroidSDKSettingsCustomization.h"
#include "ModuleInterface.h"
#include "ISettingsModule.h"
#include "ModuleManager.h"


#define LOCTEXT_NAMESPACE "FAndroidPlatformEditorModule"


/**
 * Module for Android platform editor utilities
 */
class FAndroidPlatformEditorModule
	: public IModuleInterface
{
	// IModuleInterface interface

	virtual void StartupModule() override
	{
		// register settings detail panel customization
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomClassLayout(
			UAndroidRuntimeSettings::StaticClass()->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(&FAndroidTargetSettingsCustomization::MakeInstance)
		);

		PropertyModule.RegisterCustomClassLayout(
			UAndroidSDKSettings::StaticClass()->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(&FAndroidSDKSettingsCustomization::MakeInstance)
			);

		PropertyModule.NotifyCustomizationModuleChanged();

		// register settings
		ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

		if (SettingsModule != nullptr)
		{
			SettingsModule->RegisterSettings("Project", "Platforms", "Android",
				LOCTEXT("RuntimeSettingsName", "Android"),
				LOCTEXT("RuntimeSettingsDescription", "Settings and resources for Android platforms"),
				GetMutableDefault<UAndroidRuntimeSettings>()
			);

			// This will need to be added back in once we know where we are going to be setting the settings from
// 			SettingsModule->RegisterSettings("Project", "User Settings", "Android",
// 				LOCTEXT("RuntimeSettingsName", "Android"),
// 				LOCTEXT("RuntimeSettingsDescription", "Settings for Android SDK"),
// 				GetMutableDefault<UAndroidSDKSettings>()
//			);
		}

		// Force the SDK settings into a sane state initially so we can make use of them
		auto &TargetPlatformManagerModule = FModuleManager::LoadModuleChecked<ITargetPlatformManagerModule>("TargetPlatform");
		UAndroidSDKSettings * settings = GetMutableDefault<UAndroidSDKSettings>();
		settings->SetTargetModule(&TargetPlatformManagerModule);
		auto &AndroidDeviceDetection = FModuleManager::LoadModuleChecked<IAndroidDeviceDetection>("AndroidDeviceDetection");
		settings->SetDeviceDetection(&AndroidDeviceDetection);
		settings->SetupInitialTargetPaths();

	}

	virtual void ShutdownModule() override
	{
		ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

		if (SettingsModule != nullptr)
		{
			SettingsModule->UnregisterSettings("Project", "Platforms", "Android");
			// SettingsModule->UnregisterSettings("Project", "User Settings", "Android");
		}
	}
};


IMPLEMENT_MODULE(FAndroidPlatformEditorModule, AndroidPlatformEditor);

#undef LOCTEXT_NAMESPACE
