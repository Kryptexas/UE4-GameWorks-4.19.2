// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MainMenu.cpp: Implements the FMainMenu class.
=============================================================================*/

#include "MainFramePrivatePCH.h"

#include "../../WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "GlobalEditorCommonCommands.h"
#include "Editor/UnrealEd/Public/Features/EditorFeatures.h"
#include "Runtime/Core/Public/Features/IModularFeatures.h"
#include "MultiBox/MultiBoxDefs.h"
#include "SourceCodeNavigation.h"
#include "Toolkits/AssetEditorToolkit.h"


#define LOCTEXT_NAMESPACE "MainFileMenu"

void FMainMenu::FillFileMenu( FMenuBuilder& MenuBuilder, const TSharedRef<FExtender> Extender )
{
	MenuBuilder.BeginSection("FileLoadAndSave", LOCTEXT("LoadSandSaveHeading", "Load and Save"));
	{
		// Open Asset...
		MenuBuilder.AddMenuEntry( FGlobalEditorCommonCommands::Get().SummonOpenAssetDialog );

		// Save All
		MenuBuilder.AddMenuEntry(FMainFrameCommands::Get().SaveAll, "SaveAll");

		// Choose specific files to save
		MenuBuilder.AddMenuEntry(FMainFrameCommands::Get().ChooseFilesToSave, "ChooseFilesToSave");

		if (ISourceControlModule::Get().IsEnabled() && ISourceControlModule::Get().GetProvider().IsAvailable())
		{
			// Choose specific files to submit
			MenuBuilder.AddMenuEntry(FMainFrameCommands::Get().ChooseFilesToCheckIn, "ChooseFilesToCheckIn");
		}
		else
		{
			MenuBuilder.AddMenuEntry(FMainFrameCommands::Get().ConnectToSourceControl, "ConnectToSourceControl");
		}
	}
	MenuBuilder.EndSection();
}

#undef LOCTEXT_NAMESPACE


#define LOCTEXT_NAMESPACE "MainEditMenu"

void FMainMenu::FillEditMenu( FMenuBuilder& MenuBuilder, const TSharedRef< FExtender > Extender, const TSharedPtr<FTabManager> TabManager )
{
	MenuBuilder.BeginSection("EditHistory", LOCTEXT("HistoryHeading", "History"));
	{
		struct Local
		{
			/** @return Returns a dynamic text string for Undo that contains the name of the action */
			static FText GetUndoLabelText()
			{
				return FText::Format(LOCTEXT("DynamicUndoLabel", "Undo {0}"), GUnrealEd->Trans->GetUndoContext().Title);
			}

			/** @return Returns a dynamic text string for Redo that contains the name of the action */
			static FText GetRedoLabelText()
			{
				return FText::Format(LOCTEXT("DynamicRedoLabel", "Redo {0}"), GUnrealEd->Trans->GetRedoContext().Title);
			}
		};

		// Undo
		TAttribute<FText> DynamicUndoLabel;
		DynamicUndoLabel.BindStatic(&Local::GetUndoLabelText);
		MenuBuilder.AddMenuEntry( FGenericCommands::Get().Undo, "Undo", DynamicUndoLabel); // TAttribute< FString >::Create( &Local::GetUndoLabelText ) );

		// Redo
		TAttribute< FText > DynamicRedoLabel;
		DynamicRedoLabel.BindStatic( &Local::GetRedoLabelText );
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Redo, "Redo", DynamicRedoLabel); // TAttribute< FString >::Create( &Local::GetRedoLabelText ) );
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("EditLocalTabSpawners", LOCTEXT("ConfigurationHeading", "Configuration"));
	{
		if (GetDefault<UEditorExperimentalSettings>()->bToolbarCustomization)
		{
			FUIAction ToggleMultiBoxEditMode(
				FExecuteAction::CreateStatic(&FMultiBoxSettings::ToggleToolbarEditing),
				FCanExecuteAction(),
				FIsActionChecked::CreateStatic(&FMultiBoxSettings::IsInToolbarEditMode)
			);
		
			MenuBuilder.AddMenuEntry(
				LOCTEXT("EditToolbarsLabel", "Edit Toolbars"),
				LOCTEXT("EditToolbarsToolTip", "Allows customization of each toolbar"),
				FSlateIcon(),
				ToggleMultiBoxEditMode,
				NAME_None,
				EUserInterfaceActionType::ToggleButton
			);

			// Automatically populate tab spawners from TabManager
			if (TabManager.IsValid())
			{
				const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();
				TabManager->PopulateTabSpawnerMenu(MenuBuilder, MenuStructure.GetEditOptions());
			}
		}

		if (GetDefault<UEditorStyleSettings>()->bExpandConfigurationMenus)
		{
			MenuBuilder.AddSubMenu(
				LOCTEXT("EditorPreferencesSubMenuLabel", "Editor Preferences"),
				LOCTEXT("EditorPreferencesSubMenuToolTip", "Configure the behavior and features of this Editor"),
				FNewMenuDelegate::CreateStatic(&FSettingsMenu::MakeMenu, FName("Editor"))
			);

			MenuBuilder.AddSubMenu(
				LOCTEXT("ProjectSettingsSubMenuLabel", "Project Settings"),
				LOCTEXT("ProjectSettingsSubMenuToolTip", "Change the settings of the currently loaded project"),
				FNewMenuDelegate::CreateStatic(&FSettingsMenu::MakeMenu, FName("Project"))
			);
		}
		else
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("EditorPreferencesMenuLabel", "Editor Preferences..."),
				LOCTEXT("EditorPreferencesMenuToolTip", "Configure the behavior and features of this Editor"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FSettingsMenu::OpenSettings, FName("Editor"), FName("General"), FName("Appearance")))
			);

			MenuBuilder.AddMenuEntry(
				LOCTEXT("ProjectSettingsMenuLabel", "Project Settings..."),
				LOCTEXT("ProjectSettingsMenuToolTip", "Change the settings of the currently loaded project"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FSettingsMenu::OpenSettings, FName("Project"), FName("Game"), FName("General")))
			);
		}
	}
	MenuBuilder.EndSection();
}

#undef LOCTEXT_NAMESPACE

#define LOCTEXT_NAMESPACE "MainWindowMenu"

void FMainMenu::FillWindowMenu( FMenuBuilder& MenuBuilder, const TSharedRef< FExtender > Extender, const TSharedPtr<FTabManager> TabManager )
{
	MenuBuilder.BeginSection("WindowLocalTabSpawners");
	{
		// Automatically populate tab spawners from TabManager
		if (TabManager.IsValid())
		{
			const IWorkspaceMenuStructure& MenuStructure = WorkspaceMenu::GetMenuStructure();
			TabManager->PopulateTabSpawnerMenu(MenuBuilder, MenuStructure.GetStructureRoot());
		}
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("WindowGlobalTabSpawners");
	{
		//@todo The tab system needs to be able to be extendable by plugins [9/3/2013 Justin.Sargent]
		if (IModularFeatures::Get().IsModularFeatureAvailable(EditorFeatures::PluginsEditor ) )
		{
			FGlobalTabmanager::Get()->PopulateTabSpawnerMenu(MenuBuilder, "PluginsEditor");
		}
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("UnrealFrontendTabs", NSLOCTEXT("MainAppMenu", "UnrealFrontendHeader", "Unreal Frontend"));
	{
		FGlobalTabmanager::Get()->PopulateTabSpawnerMenu(MenuBuilder, "DeviceManager");

		if (GetDefault<UEditorExperimentalSettings>()->bMessagingDebugger)
		{
			if (IModularFeatures::Get().IsModularFeatureAvailable("MessagingDebugger"))
			{
				FGlobalTabmanager::Get()->PopulateTabSpawnerMenu(MenuBuilder, "MessagingDebugger");
			}
		}

		FGlobalTabmanager::Get()->PopulateTabSpawnerMenu(MenuBuilder, "SessionFrontend");

		if (GetDefault<UEditorExperimentalSettings>()->bGameLauncher)
		{
			FGlobalTabmanager::Get()->PopulateTabSpawnerMenu(MenuBuilder, "SessionLauncher");
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TranslationTools")))
		{
			MenuBuilder.AddSubMenu(
				LOCTEXT("TranslationEditorSubMenuLabel", "TranslationEditor"),
				LOCTEXT("EditorPreferencesSubMenuToolTip", "Open the Translation Editor for a Given Project and Language"),
				FNewMenuDelegate::CreateStatic(&FMainFrameTranslationEditorMenu::MakeMainFrameTranslationEditorSubMenu)
			);
		}
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("WindowLayout", NSLOCTEXT("MainAppMenu", "LayoutManagementHeader", "Layout"));
	{
		MenuBuilder.AddMenuEntry(FMainFrameCommands::Get().SaveLayout);
		MenuBuilder.AddMenuEntry(FMainFrameCommands::Get().ToggleFullscreen);
	}
	MenuBuilder.EndSection();
}

#undef LOCTEXT_NAMESPACE


void FMainMenu::FillHelpMenu( FMenuBuilder& MenuBuilder, const TSharedRef< FExtender > Extender )
{
	MenuBuilder.BeginSection("HelpOnline", NSLOCTEXT("MainHelpMenu", "Online", "Online"));
	{
		MenuBuilder.AddMenuEntry(FMainFrameCommands::Get().VisitForums);
		MenuBuilder.AddMenuEntry(FMainFrameCommands::Get().VisitSearchForAnswersPage);
		MenuBuilder.AddMenuEntry(FMainFrameCommands::Get().VisitWiki);


		const FText SupportWebSiteLabel = NSLOCTEXT("MainHelpMenu", "VisitUnrealEngineSupportWebSite", "Unreal Engine Support Web Site...");

		MenuBuilder.AddMenuSeparator("EpicGamesHelp");
		MenuBuilder.AddMenuEntry(FMainFrameCommands::Get().VisitEpicGamesDotCom, "VisitEpicGamesDotCom");
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("HelpApplication", NSLOCTEXT("MainHelpMenu", "Application", "Application"));
	{
		const FText AboutWindowTitle = NSLOCTEXT("MainHelpMenu", "AboutUnrealEditor", "About Unreal Editor...");

		MenuBuilder.AddMenuEntry(FMainFrameCommands::Get().AboutUnrealEd, "AboutUnrealEd", AboutWindowTitle);
	}
	MenuBuilder.EndSection();
}


TSharedRef<SWidget> FMainMenu::MakeMainMenu( const TSharedPtr<FTabManager>& TabManager, const TSharedRef< FExtender > Extender )
{
#define LOCTEXT_NAMESPACE "MainMenu"

	// Cache all project names once
	FMainFrameActionCallbacks::CacheProjectNames();

	FMenuBarBuilder MenuBuilder(FMainFrameCommands::ActionList, Extender);
	{
		// File
		MenuBuilder.AddPullDownMenu( 
			LOCTEXT("FileMenu", "File"),
			LOCTEXT("FileMenu_ToolTip", "Open the file menu"),
			FNewMenuDelegate::CreateStatic(&FMainMenu::FillFileMenu, Extender),
			"File",
			FName(TEXT("FileMenu"))
		);

		// Edit
		MenuBuilder.AddPullDownMenu( 
			LOCTEXT("EditMenu", "Edit"),
			LOCTEXT("EditMenu_ToolTip", "Open the edit menu"),
			FNewMenuDelegate::CreateStatic(&FMainMenu::FillEditMenu, Extender, TabManager),
			"Edit"
		);

		// Window
		MenuBuilder.AddPullDownMenu(
			LOCTEXT("WindowMenu", "Window"),
			LOCTEXT("WindowMenu_ToolTip", "Open new windows or tabs."),
			FNewMenuDelegate::CreateStatic(&FMainMenu::FillWindowMenu, Extender, TabManager),
			"Window"
		);

		// Help
		MenuBuilder.AddPullDownMenu( 
			LOCTEXT("HelpMenu", "Help"),
			LOCTEXT("HelpMenu_ToolTip", "Open the help menu"),
			FNewMenuDelegate::CreateStatic(&FMainMenu::FillHelpMenu, Extender),
			"Help"
		);
	}

	// Create the menu bar!
	TSharedRef<SWidget> MenuBarWidget = MenuBuilder.MakeWidget();

	return MenuBarWidget;
}

#undef LOCTEXT_NAMESPACE


#define LOCTEXT_NAMESPACE "MainTabMenu"

TSharedRef< SWidget > FMainMenu::MakeMainTabMenu( const TSharedPtr<FTabManager>& TabManager, const TSharedRef< FExtender > UserExtender )
{	struct Local
	{
		static void FillProjectMenuItems( FMenuBuilder& MenuBuilder )
		{
			MenuBuilder.BeginSection( "FileProject", LOCTEXT("ProjectHeading", "Project") );
			{
				MenuBuilder.AddMenuEntry( FMainFrameCommands::Get().NewProject );
				MenuBuilder.AddMenuEntry( FMainFrameCommands::Get().OpenProject );

				const bool bUseShortIDEName = true;
				FText ShortIDEName = FSourceCodeNavigation::GetSuggestedSourceCodeIDE(bUseShortIDEName);

				MenuBuilder.AddMenuEntry( FMainFrameCommands::Get().AddCodeToProject,
					NAME_None,
					TAttribute<FText>(),
					FText::Format(LOCTEXT("AddCodeToProjectTooltip", "Adds C++ code to the project. The code can only be compiled if you have {0} installed."), ShortIDEName)
				);

				MenuBuilder.AddSubMenu(
					LOCTEXT("PackageProjectSubMenuLabel", "Package Project"),
					LOCTEXT("PackageProjectSubMenuToolTip", "Compile, cook and package your project and its content for distribution"),
					FNewMenuDelegate::CreateStatic( &FPackageProjectMenu::MakeMenu ), false, FSlateIcon(FEditorStyle::GetStyleSetName(), "MainFrame.PackageProject")
				);

				if (FPaths::FileExists(FModuleManager::Get().GetSolutionFilepath()))
				{
					MenuBuilder.AddMenuEntry( FMainFrameCommands::Get().RefreshCodeProject,
						NAME_None,
						FText::Format(LOCTEXT("RefreshCodeProjectLabel", "Refresh {0} Project"), ShortIDEName),
						FText::Format(LOCTEXT("RefreshCodeProjectTooltip", "Refreshes your C++ code project in {0}."), ShortIDEName)
					);

					MenuBuilder.AddMenuEntry( FMainFrameCommands::Get().OpenIDE,
						NAME_None,
						FText::Format(LOCTEXT("OpenIDELabel", "Open {0}"), ShortIDEName),
						FText::Format(LOCTEXT("OpenIDETooltip", "Opens your C++ code in {0}."), ShortIDEName)
					);
				}

				// @hack GDC: this should be moved somewhere else and be less hacky
				ITargetPlatform* RunningTargetPlatform = GetTargetPlatformManager()->GetRunningTargetPlatform();

				if (RunningTargetPlatform != nullptr)
				{
					FString CookedPlatformName = RunningTargetPlatform->PlatformName() + TEXT("NoEditor");
					FText CookedPlatformText = FText::FromString(RunningTargetPlatform->PlatformName());

					FUIAction Action(
						FExecuteAction::CreateStatic(&FMainFrameActionCallbacks::CookContent, CookedPlatformName, CookedPlatformText),
						FCanExecuteAction::CreateStatic(&FMainFrameActionCallbacks::CookContentCanExecute, CookedPlatformName)
					);

					MenuBuilder.AddMenuEntry(
						FText::Format(LOCTEXT("CookContentForPlatform", "Cook Content for {0}"), CookedPlatformText),
						FText::Format(LOCTEXT("CookContentForPlatformTooltip", "Cook your game content for debugging on the {0} platform"), CookedPlatformText),
						FSlateIcon(),
						Action
					);
				}
			}
			MenuBuilder.EndSection();
		}

		static void FillRecentFileAndExitMenuItems( FMenuBuilder& MenuBuilder )
		{
			MenuBuilder.BeginSection( "FileRecentFiles" );
			{
				if ( FMainFrameActionCallbacks::ProjectNames.Num() > 0 )
				{
					MenuBuilder.AddSubMenu(
						LOCTEXT("SwitchProjectSubMenu", "Recent Projects"),
						LOCTEXT("SwitchProjectSubMenu_ToolTip", "Select a project to switch to"),
						FNewMenuDelegate::CreateStatic( &FRecentProjectsMenu::MakeMenu ), false, FSlateIcon(FEditorStyle::GetStyleSetName(), "MainFrame.RecentProjects")
					);
				}
			}
			MenuBuilder.EndSection();

			MenuBuilder.AddMenuSeparator();
			MenuBuilder.AddMenuEntry( FMainFrameCommands::Get().Exit, "Exit" );
		}
	};

	FExtensibilityManager ExtensibilityManager;

	ExtensibilityManager.AddExtender( UserExtender );
	{
		TSharedRef< FExtender > Extender( new FExtender() );
	
		IMainFrameModule& MainFrameModule = FModuleManager::GetModuleChecked<IMainFrameModule>("MainFrame");

		Extender->AddMenuExtension(
			"FileLoadAndSave",
			EExtensionHook::After,
			MainFrameModule.GetMainFrameCommandBindings(),
			FMenuExtensionDelegate::CreateStatic( &Local::FillProjectMenuItems ) );

		Extender->AddMenuExtension(
			"FileLoadAndSave",
			EExtensionHook::After,
			MainFrameModule.GetMainFrameCommandBindings(),
			FMenuExtensionDelegate::CreateStatic( &Local::FillRecentFileAndExitMenuItems ) );

		ExtensibilityManager.AddExtender( Extender );
	}


	TSharedRef< SWidget > MenuBarWidget = FMainMenu::MakeMainMenu( TabManager, ExtensibilityManager.GetAllExtenders().ToSharedRef() );

	return MenuBarWidget;
}


#undef LOCTEXT_NAMESPACE
