// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "CryptoKeys.h"
#include "ModuleManager.h"
#include "ISettingsModule.h"
#include "PropertyEditorModule.h"
#include "CryptoKeysSettings.h"
#include "CryptoKeysSettingsDetails.h"

#define LOCTEXT_NAMESPACE "CryptoKeysModule"

class FCryptoKeysModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override
	{
		RegisterSettings();

		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomClassLayout(UCryptoKeysSettings::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FCryptoKeysSettingsDetails::MakeInstance));
	}

	virtual void ShutdownModule() override
	{
		if (UObjectInitialized())
		{
			UnregisterSettings();

			FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
			PropertyModule.UnregisterCustomClassLayout(UCryptoKeysSettings::StaticClass()->GetFName());
		}
	}

	void RegisterSettings()
	{
		if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
		{
			SettingsModule->RegisterSettings("Project", "Project", "Crypto",
				LOCTEXT("CryptoSettingsName", "Crypto"),
				LOCTEXT("CryptoSettingsDescription", "Configure the project crypto keys"),
				GetMutableDefault<UCryptoKeysSettings>());
		}
	}

	void UnregisterSettings()
	{
		if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
		{
			SettingsModule->UnregisterSettings("Project", "Project", "Crypto");
		}
	}
};

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCryptoKeysModule, CryptoKeys)