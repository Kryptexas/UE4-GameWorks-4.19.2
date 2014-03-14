// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	EditorSettingsViewerModule.cpp: Implements the FEditorSettingsViewerModule class.
=============================================================================*/

#include "EditorSettingsViewerPrivatePCH.h"


#define LOCTEXT_NAMESPACE "FEditorSettingsViewerModule"

static const FName EditorSettingsTabName("EditorSettings");


/**
 * Implements the EditorSettingsViewer module.
 */
class FEditorSettingsViewerModule
	: public IModuleInterface
	, public ISettingsViewer
{
public:

	// Begin ISettingsViewer interface

	virtual void ShowSettings( const FName& CategoryName, const FName& SectionName ) OVERRIDE
	{
		FGlobalTabmanager::Get()->InvokeTab(EditorSettingsTabName);
		ISettingsEditorModelPtr SettingsEditorModel = SettingsEditorModelPtr.Pin();

		if (SettingsEditorModel.IsValid())
		{
			ISettingsCategoryPtr Category = SettingsEditorModel->GetSettingsContainer()->GetCategory(CategoryName);

			if (Category.IsValid())
			{
				SettingsEditorModel->SelectSection(Category->GetSection(SectionName));
			}
		}
	}

	// End ISettingsViewer interface

public:

	// Begin IModuleInterface interface

	virtual void StartupModule( ) OVERRIDE
	{
		ISettingsModule* SettingsModule = ISettingsModule::Get();

		if (SettingsModule != nullptr)
		{
			RegisterGeneralSettings(*SettingsModule);
			RegisterLevelEditorSettings(*SettingsModule);
			RegisterContentEditorsSettings(*SettingsModule);

			SettingsModule->RegisterViewer("Editor", *this);
		}

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(EditorSettingsTabName, FOnSpawnTab::CreateRaw(this, &FEditorSettingsViewerModule::HandleSpawnSettingsTab))
			.SetDisplayName(LOCTEXT("EditorSettingsTabTitle", "Editor Preferences"))
			.SetMenuType(ETabSpawnerMenuType::Hide);
	}

	virtual void ShutdownModule( ) OVERRIDE
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(EditorSettingsTabName);
		UnregisterSettings();
	}

	virtual bool SupportsDynamicReloading( ) OVERRIDE
	{
		return true;
	}

	// End IModuleInterface interface

protected:

	/**
	 * Registers general Editor settings.
	 */
	void RegisterGeneralSettings( ISettingsModule& SettingsModule )
	{
		// general settings
		FSettingsSectionDelegates UserSettingsDelegates;
		{
			UserSettingsDelegates.ExportDelegate = FOnSettingsSectionExport::CreateRaw(this, &FEditorSettingsViewerModule::HandleUserSettingsExport);
			UserSettingsDelegates.ImportDelegate = FOnSettingsSectionImport::CreateRaw(this, &FEditorSettingsViewerModule::HandleUserSettingsImport);
			UserSettingsDelegates.ResetDefaultsDelegate = FOnSettingsSectionResetDefaults::CreateRaw(this, &FEditorSettingsViewerModule::HandleUserSettingsResetDefaults);
		}

		// input bindings
		FSettingsSectionDelegates InputBindingDelegates;
		InputBindingDelegates.ExportDelegate = FOnSettingsSectionExport::CreateRaw(this, &FEditorSettingsViewerModule::HandleInputBindingsExport);
		InputBindingDelegates.ImportDelegate = FOnSettingsSectionImport::CreateRaw(this, &FEditorSettingsViewerModule::HandleInputBindingsImport);
		InputBindingDelegates.ResetDefaultsDelegate = FOnSettingsSectionResetDefaults::CreateRaw(this, &FEditorSettingsViewerModule::HandleInputBindingsResetToDefault);

		InputBindingEditorPanel = FModuleManager::LoadModuleChecked<IInputBindingEditorModule>("InputBindingEditor").CreateInputBindingEditorPanel();
		SettingsModule.RegisterSettings("Editor", "General", "InputBindings",
			LOCTEXT("InputBindingsSettingsName", "Keyboard Shortcuts"),
			LOCTEXT("InputBindingsSettingsDescription", "Configure keyboard shortcuts to quickly invoke operations."),
			InputBindingEditorPanel.Pin().ToSharedRef(),
			InputBindingDelegates
		);

		// loading & saving features
		SettingsModule.RegisterSettings("Editor", "General", "LoadingSaving",
			LOCTEXT("LoadingSavingSettingsName", "Loading & Saving"),
			LOCTEXT("LoadingSavingSettingsDescription", "Change how the Editor loads and saves files."),
			TWeakObjectPtr<UObject>(GetMutableDefault<UEditorLoadingSavingSettings>()),
			UserSettingsDelegates
		);

		// @todo thomass: proper settings support for source control module
		GetMutableDefault<UEditorLoadingSavingSettings>()->SccHackInitialize();

		// misc unsorted settings
		SettingsModule.RegisterSettings("Editor", "General", "UserSettings",
			LOCTEXT("UserSettingsName", "Miscellaneous"),
			LOCTEXT("UserSettingsDescription", "Customize the behavior, look and feel of the editor."),
			TWeakObjectPtr<UObject>(&GEditor->AccessEditorUserSettings()),
			UserSettingsDelegates
		);

		// misc unsorted settings
		SettingsModule.RegisterSettings("Editor", "General", "AutomationTest",
			LOCTEXT("AutomationSettingsName", "AutomationTest"),
			LOCTEXT("AutomationSettingsDescription", "Set up automation test assets."),
			TWeakObjectPtr<UObject>(GetMutableDefault<UAutomationTestSettings>())
			);

		// experimental features
		SettingsModule.RegisterSettings("Editor", "General", "Experimental",
			LOCTEXT("ExperimentalettingsName", "Experimental"),
			LOCTEXT("ExperimentalSettingsDescription", "Enable and configure experimental Editor features."),
			TWeakObjectPtr<UObject>(GetMutableDefault<UEditorExperimentalSettings>())
		);
	}

	/**
	 * Registers Level Editor settings
	 */
	void RegisterLevelEditorSettings( ISettingsModule& SettingsModule )
	{
		// play-in settings
		SettingsModule.RegisterSettings("Editor", "LevelEditor", "PlayIn",
			LOCTEXT("LevelEditorPlaySettingsName", "Play"),
			LOCTEXT("LevelEditorPlaySettingsDescription", "Set up window sizes and other options for the Play In Editor (PIE) feature."),
			TWeakObjectPtr<UObject>(GetMutableDefault<ULevelEditorPlaySettings>())
		);

		// view port settings
		SettingsModule.RegisterSettings("Editor", "LevelEditor", "Viewport",
			LOCTEXT("LevelEditorViewportSettingsName", "Viewports"),
			LOCTEXT("LevelEditorViewportSettingsDescription", "Configure the look and feel of the Level Editor view ports."),
			TWeakObjectPtr<UObject>(GetMutableDefault<ULevelEditorViewportSettings>())
		);
	}

	/**
	 * Registers Other Tools settings
	 */
	void RegisterContentEditorsSettings( ISettingsModule& SettingsModule )
	{
		// content browser
		SettingsModule.RegisterSettings("Editor", "ContentEditors", "ContentBrowser",
			LOCTEXT("ContentEditorsContentBrowserSettingsName", "Content Browser"),
			LOCTEXT("ContentEditorsContentBrowserSettingsDescription", "Change the behavior of the Content Browser."),
			TWeakObjectPtr<UObject>(GetMutableDefault<UContentBrowserSettings>())
		);

		// destructable mesh editor
/*		SettingsModule.RegisterSettings("Editor", "ContentEditors", "DestructableMeshEditor",
			LOCTEXT("ContentEditorsDestructableMeshEditorSettingsName", "Destructable Mesh Editor"),
			LOCTEXT("ContentEditorsDestructableMeshEditorSettingsDescription", "Change the behavior of the Destructable Mesh Editor."),
			TWeakObjectPtr<UObject>(GetMutableDefault<UDestructableMeshEditorSettings>())
		);*/
	}

	/**
	 * Unregisters all settings.
	 */
	void UnregisterSettings( )
	{
		ISettingsModule* SettingsModule = ISettingsModule::Get();

		if (SettingsModule != nullptr)
		{
			SettingsModule->UnregisterViewer("Editor");

			// general settings
			SettingsModule->UnregisterSettings("Editor", "General", "InputBindings");
			SettingsModule->UnregisterSettings("Editor", "General", "LoadingSaving");
			SettingsModule->UnregisterSettings("Editor", "General", "GameAgnostic");
			SettingsModule->UnregisterSettings("Editor", "General", "UserSettings");
			SettingsModule->UnregisterSettings("Editor", "General", "AutomationTest");
			SettingsModule->UnregisterSettings("Editor", "General", "Experimental");

			// level editor settings
			SettingsModule->UnregisterSettings("Editor", "LevelEditor", "PlayIn");
			SettingsModule->UnregisterSettings("Editor", "LevelEditor", "Viewport");

			// other tools
			SettingsModule->UnregisterSettings("Editor", "ContentEditors", "ContentBrowser");
//			SettingsModule->UnregisterSettings("Editor", "ContentEditors", "DestructableMeshEditor");
		}
	}

private:

	// Handles creating the editor settings tab.
	TSharedRef<SDockTab> HandleSpawnSettingsTab( const FSpawnTabArgs& SpawnTabArgs )
	{
		ISettingsModule* SettingsModule = ISettingsModule::Get();
		TSharedRef<SWidget> SettingsEditor = SNullWidget::NullWidget;

		if (SettingsModule != nullptr)
		{
			ISettingsContainerPtr SettingsContainer = SettingsModule->GetContainer("Editor");

			if (SettingsContainer.IsValid())
			{
				ISettingsEditorModule& SettingsEditorModule = ISettingsEditorModule::GetRef();
				ISettingsEditorModelRef SettingsEditorModel = SettingsEditorModule.CreateModel(SettingsContainer.ToSharedRef());

				SettingsEditor = SettingsEditorModule.CreateEditor(SettingsEditorModel);
				SettingsEditorModelPtr = SettingsEditorModel;
			}
		}

		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			[
				SettingsEditor
			];
	}

private:

	// Show a warning that the editor will require a restart and return its result
	EAppReturnType::Type ShowRestartWarning(const FText& Title) const
	{
		return OpenMsgDlgInt(EAppMsgType::OkCancel, LOCTEXT("ActionRestartMsg", "This action requires the editor to restart; you will be prompted to save any changes. Continue?" ), Title);
	}

	// Backup a file
	bool BackupFile(const FString& SrcFilename, const FString& DstFilename)
	{
		if (IFileManager::Get().Copy(*DstFilename, *SrcFilename) == COPY_OK)
		{
			return true;
		}

		// log error	
		FMessageLog EditorErrors("EditorErrors");
		if(!FPaths::FileExists(SrcFilename))
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("FileName"), FText::FromString(SrcFilename));
			EditorErrors.Warning(FText::Format(LOCTEXT("UnsuccessfulBackup_NoExist_Notification", "Unsuccessful backup! {FileName} does not exist!"), Arguments));
		}
		else if(IFileManager::Get().IsReadOnly(*DstFilename))
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("FileName"), FText::FromString(DstFilename));
			EditorErrors.Warning(FText::Format(LOCTEXT("UnsuccessfulBackup_ReadOnly_Notification", "Unsuccessful backup! {FileName} is read-only!"), Arguments));
		}
		else
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("SourceFileName"), FText::FromString(SrcFilename));
			Arguments.Add(TEXT("BackupFileName"), FText::FromString(DstFilename));
			// We don't specifically know why it failed, this is a fallback.
			EditorErrors.Warning(FText::Format(LOCTEXT("UnsuccessfulBackup_Fallback_Notification", "Unsuccessful backup of {SourceFileName} to {BackupFileName}"), Arguments));
		}
		EditorErrors.Notify(LOCTEXT("BackupUnsuccessful_Title", "Backup Unsuccessful!"));	

		return false;
	}

	// Handles exporting the preferences settings to a file.
	bool HandleUserSettingsExport( const FString& Filename )
	{
		FGlobalTabmanager::Get()->SaveAllVisualState();
		GEditor->SaveEditorUserSettings();
		return BackupFile(GEditorUserSettingsIni, *Filename);
	}

	// Handles importing the preferences settings from a file.
	bool HandleUserSettingsImport( const FString& Filename )
	{
		if( EAppReturnType::Ok == ShowRestartWarning(LOCTEXT("ImportPreferences_Title", "Import Preferences")))
		{
			FUnrealEdMisc::Get().SetConfigRestoreFilename(Filename, GEditorUserSettingsIni);
			FUnrealEdMisc::Get().RestartEditor(false);

			return true;
		}

		return false;
	}

	// Handles resetting the preferences settings to their defaults.
	bool HandleUserSettingsResetDefaults( )
	{
		if( EAppReturnType::Ok == ShowRestartWarning(LOCTEXT("ResetPreferences_Title", "Reset Preferences")))
		{
			FUnrealEdMisc::Get().ForceDeletePreferences(true);
			FUnrealEdMisc::Get().RestartEditor(false);

			return true;
		};

		return false;
	}

	// Handles exporting input bindings to a file
	bool HandleInputBindingsExport( const FString& Filename )
	{
		FInputBindingManager::Get().SaveInputBindings();
		GConfig->Flush(false, GEditorKeyBindingsIni);
		return BackupFile(GEditorKeyBindingsIni, Filename);
	}

	// Handles importing input bindings from a file
	bool HandleInputBindingsImport( const FString& Filename )
	{
		if( EAppReturnType::Ok == ShowRestartWarning(LOCTEXT("ImportKeyBindings_Title", "Import Key Bindings")))
		{
			FUnrealEdMisc::Get().SetConfigRestoreFilename(Filename, GEditorKeyBindingsIni);
			FUnrealEdMisc::Get().RestartEditor(false);

			return true;
		}

		return false;
	}

	// Handles resetting input bindings back to the defaults
	bool HandleInputBindingsResetToDefault()
	{
		if( EAppReturnType::Ok == ShowRestartWarning(LOCTEXT("ResetKeyBindings_Title", "Reset Key Bindings")))
		{
			FInputBindingManager::Get().RemoveUserDefinedGestures();
			GConfig->Flush(false, GEditorKeyBindingsIni);
			FUnrealEdMisc::Get().RestartEditor(false);

			return true;
		}

		return false;
	}

private:

	// Holds a pointer to the input binding editor.
	TWeakPtr<SWidget> InputBindingEditorPanel;

	// Holds a pointer to the settings editor's view model.
	TWeakPtr<ISettingsEditorModel> SettingsEditorModelPtr;
};


IMPLEMENT_MODULE(FEditorSettingsViewerModule, EditorSettingsViewer);


#undef LOCTEXT_NAMESPACE
