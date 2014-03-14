// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ProjectSettingsViewerModule.cpp: Implements the FProjectSettingsViewerModule class.
=============================================================================*/

#include "ProjectSettingsViewerPrivatePCH.h"


#define LOCTEXT_NAMESPACE "FProjectSettingsViewerModule"

static const FName ProjectSettingsTabName("ProjectSettings");


/**
 * Implements the ProjectSettingsViewer module.
 */
class FProjectSettingsViewerModule
	: public IModuleInterface
	, public ISettingsViewer
{
public:

	// Begin ISettingsViewer interface

	virtual void ShowSettings( const FName& CategoryName, const FName& SectionName ) OVERRIDE
	{
		FGlobalTabmanager::Get()->InvokeTab(ProjectSettingsTabName);
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
			RegisterEngineSettings(*SettingsModule);
			RegisterGameSettings(*SettingsModule);

			SettingsModule->RegisterViewer("Project", *this);
		}

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ProjectSettingsTabName, FOnSpawnTab::CreateRaw(this, &FProjectSettingsViewerModule::HandleSpawnSettingsTab))
			.SetDisplayName(LOCTEXT("ProjectSettingsTabTitle", "Project Settings"))
			.SetMenuType(ETabSpawnerMenuType::Hide);
	}

	virtual void ShutdownModule( ) OVERRIDE
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ProjectSettingsTabName);
		UnregisterSettings();
	}

	virtual bool SupportsDynamicReloading( ) OVERRIDE
	{
		return true;
	}

	// End IModuleInterface interface


protected:

	/**
	 * Registers Engine settings.
	 */
	void RegisterEngineSettings( ISettingsModule& SettingsModule )
	{
		// startup settings
		SettingsModule.RegisterSettings("Project", "Engine", "General",
			LOCTEXT("GeneralEngineSettingsName", "General Settings"),
			LOCTEXT("ProjectGeneralSettingsDescription", "General options and defaults for the game engine."),
			TWeakObjectPtr<UObject>(GetMutableDefault<UEngine>())
		);

		// packaging settings
		SettingsModule.RegisterSettings("Project", "Engine", "Collision",
			LOCTEXT("ProjectCollisionSettingsName", "Collision"),
			LOCTEXT("ProjectCollisionSettingsDescription", "Set up and modify collision settings."),
			TWeakObjectPtr<UObject>(GetMutableDefault<UCollisionProfile>())
		);

		// game console settings
		SettingsModule.RegisterSettings("Project", "Engine", "Console",
			LOCTEXT("ProjectConsoleSettingsName", "Console"),
			LOCTEXT("ProjectConsoleSettingsDescription", "Configure the in-game input console."),
			TWeakObjectPtr<UObject>(GetMutableDefault<UConsole>())
		);

		// Input bindings
		SettingsModule.RegisterSettings("Project", "Engine", "Input",
			LOCTEXT("EngineInputSettingsName", "Input"),
			LOCTEXT("ProjectInputSettingsDescription", "Input settings, including default input action and axis bindings."),
			TWeakObjectPtr<UObject>(GetMutableDefault<UInputSettings>())
		);

		// navigation system
		SettingsModule.RegisterSettings("Project", "Engine", "NavigationSystem",
			LOCTEXT("NavigationSystemSettingsName", "Navigation System"),
			LOCTEXT("NavigationSystemSettingsDescription", "Settings for the navigation system."),
			TWeakObjectPtr<UObject>(GetMutableDefault<UNavigationSystem>())
		);

		// navigation mesh
		SettingsModule.RegisterSettings("Project", "Engine", "NavigationMesh",
			LOCTEXT("NavigationMeshSettingsName", "Navigation Mesh"),
			LOCTEXT("NavigationMeshSettingsDescription", "Settings for the navigation mesh."),
			TWeakObjectPtr<UObject>(GetMutableDefault<ARecastNavMesh>())
		);

/*
		// network settings
		SettingsModule.RegisterSettings("Project", "Engine", "NetworkManager",
			LOCTEXT("GameNetworkManagerSettingsName", "Network Manager"),
			LOCTEXT("GameNetworkManagerSettingsDescription", "Game Network Manager settings."),
			TWeakObjectPtr<UObject>(GetMutableDefault<UGameNetworkManagerSettings>())
		);*/

		// Physics settings
		SettingsModule.RegisterSettings("Project", "Engine", "Physics",
			LOCTEXT("EnginePhysicsSettingsName", "Physics"),
			LOCTEXT("ProjectPhysicsSettingsDescription", "Default physics settings."),
			TWeakObjectPtr<UObject>(GetMutableDefault<UPhysicsSettings>())
			);	

		// Rendering settings
		SettingsModule.RegisterSettings("Project", "Engine", "Rendering",
			LOCTEXT("EngineRenderingSettingsName", "Rendering"),
			LOCTEXT("ProjectRenderingSettingsDescription", "Rendering settings."),
			TWeakObjectPtr<UObject>(GetMutableDefault<URendererSettings>())
			);	
	}

	/**
	 * Registers Game settings.
	 */
	void RegisterGameSettings( ISettingsModule& SettingsModule )
	{
		// general project settings
		SettingsModule.RegisterSettings("Project", "Game", "General",
			LOCTEXT("GeneralGameSettingsName", "Description"),
			LOCTEXT("GeneralGameSettingsDescription", "Descriptions and other information about your game."),
			TWeakObjectPtr<UObject>(GetMutableDefault<UGeneralProjectSettings>())
		);

		// map related settings
		SettingsModule.RegisterSettings("Project", "Game", "Maps",
			LOCTEXT("GameMapsSettingsName", "Maps & Modes"),
			LOCTEXT("GameMapsSettingsDescription", "Default maps, game modes and other map related settings."),
			TWeakObjectPtr<UObject>(GetMutableDefault<UGameMapsSettings>())
		);

		// packaging settings
		SettingsModule.RegisterSettings("Project", "Game", "Packaging",
			LOCTEXT("ProjectPackagingSettingsName", "Packaging"),
			LOCTEXT("ProjectPackagingSettingsDescription", "Fine tune how your project is packaged for release."),
			TWeakObjectPtr<UObject>(GetMutableDefault<UProjectPackagingSettings>())
		);

		// update the visible packaging enum values
		UProjectPackagingSettings::UpdateBuildConfigurationVisibility();
/*
		// game session
		SettingsModule.RegisterSettings("Project", "Game", "GameSession",
			LOCTEXT("GameSessionettingsName", "Game Session"),
			LOCTEXT("GameSessionSettingsDescription", "Game Session settings."),
			TWeakObjectPtr<UObject>(GetMutableDefault<UGameSessionSettings>())
		);

		// head-up display
		SettingsModule.RegisterSettings("Project", "Game", "HUD",
			LOCTEXT("HudSettingsName", "HUD"),
			LOCTEXT("HudSettingsDescription", "Head-up display (HUD) settings."),
			TWeakObjectPtr<UObject>(GetMutableDefault<UHudSettings>())
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
			SettingsModule->UnregisterViewer("Project");

			// engine settings
			SettingsModule->UnregisterSettings("Project", "Engine", "General");
			SettingsModule->UnregisterSettings("Project", "Engine", "NavigationSystem");
			SettingsModule->UnregisterSettings("Project", "Engine", "NavigationMesh");
			SettingsModule->UnregisterSettings("Project", "Engine", "Input");
			SettingsModule->UnregisterSettings("Project", "Game", "Collision");
			SettingsModule->UnregisterSettings("Project", "Engine", "Physics");
			SettingsModule->UnregisterSettings("Project", "Engine", "Rendering");
//			SettingsModule->UnregisterSettings("Project", "Engine", "NetworkManager");

			// game settings
			SettingsModule->UnregisterSettings("Project", "Game", "General");
			SettingsModule->UnregisterSettings("Project", "Game", "Maps");
			SettingsModule->UnregisterSettings("Project", "Game", "Packaging");
//			SettingsModule->UnregisterSettings("Project", "Game", "GameSession");
//			SettingsModule->UnregisterSettings("Project", "Game", "HUD");
		}
	}


private:

	// Handles creating the project settings tab.
	TSharedRef<SDockTab> HandleSpawnSettingsTab( const FSpawnTabArgs& SpawnTabArgs )
	{
		ISettingsModule* SettingsModule = ISettingsModule::Get();
		TSharedRef<SWidget> SettingsEditor = SNullWidget::NullWidget;

		if (SettingsModule != nullptr)
		{
			ISettingsContainerPtr SettingsContainer = SettingsModule->GetContainer("Project");

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

	// Holds a pointer to the settings editor's view model.
	TWeakPtr<ISettingsEditorModel> SettingsEditorModelPtr;
};


IMPLEMENT_MODULE(FProjectSettingsViewerModule, ProjectSettingsViewer);


#undef LOCTEXT_NAMESPACE
