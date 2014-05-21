// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PackageProjectMenu.h: Declares the FPackageProjectMenu class.
=============================================================================*/

#pragma once


#define LOCTEXT_NAMESPACE "FPackageProjectMenu"


/**
 * Static helper class for populating the "Package Project" menu.
 */
class FPackageProjectMenu
{
public:

	/**
	 * Creates the menu.
	 *
	 * @param MenuBuilder - The builder for the menu that owns this menu.
	 */
	static void MakeMenu( FMenuBuilder& MenuBuilder )
	{
		TArray<ITargetPlatform*> Platforms = GetTargetPlatformManager()->GetTargetPlatforms();

		if (Platforms.Num() == 0)
		{
			return;
		}
					
		for (int32 PlatformIndex = 0; PlatformIndex < Platforms.Num(); ++PlatformIndex)
		{
			const ITargetPlatform* Platform = Platforms[PlatformIndex];

			// for the Editor we are only interested in packaging standalone games
			if (Platform->IsClientOnly() || Platform->IsServerOnly() || Platform->HasEditorOnlyData())
			{
				continue;
			}

			if (Platform->PlatformName() == TEXT("WindowsNoEditor"))
			{
				TArray< FText > CollectivePlatforms;
				CollectivePlatforms.Add(LOCTEXT("Win32","Win32"));
				CollectivePlatforms.Add(LOCTEXT("Win64","Win64"));

				MenuBuilder.AddSubMenu(
					Platform->DisplayName(),
					Platform->DisplayName(),
					FNewMenuDelegate::CreateStatic(&FPackageProjectMenu::AddPlatformSubPlatformsToMenu, Platform, CollectivePlatforms),
					false,
					FSlateIcon(FEditorStyle::GetStyleSetName(), *FString::Printf(TEXT("Launcher.Platform_%s"), *Platform->PlatformName()))
					);
			}
			else
			{
				AddPlatformToMenu(MenuBuilder, Platform);
			}
		}

		MenuBuilder.AddMenuSeparator();
		MenuBuilder.AddSubMenu(
			LOCTEXT("PackageProjectBuildConfigurationSubMenuLabel", "Build Configuration"),
			LOCTEXT("PackageProjectBuildConfigurationSubMenuToolTip", "Select the build configuration to package the project with"),
			FNewMenuDelegate::CreateStatic(&FPackageProjectMenu::MakeBuildConfigurationsMenu)
		);

		MenuBuilder.AddMenuSeparator();
		MenuBuilder.AddMenuEntry(FMainFrameCommands::Get().PackagingSettings);
	}

protected:

	/**
	* Creates the platform menu entries.
	*
	* @param MenuBuilder	- The builder for the menu that owns this menu.
	* @param Platform		- The target platform we allow packaging for
	*/
	static void AddPlatformToMenu(FMenuBuilder& MenuBuilder, const ITargetPlatform* Platform)
	{
		FUIAction Action(
			FExecuteAction::CreateStatic(&FMainFrameActionCallbacks::PackageProject, Platform->PlatformName(), Platform->DisplayName(), FString()),
			FCanExecuteAction::CreateStatic(&FMainFrameActionCallbacks::PackageProjectCanExecute, Platform->PlatformName(), Platform->SupportsFeature(ETargetPlatformFeatures::Packaging))
			);

		MenuBuilder.AddMenuEntry(
			Platform->DisplayName(),
			FText::Format(LOCTEXT("PackageGameForPlatformTooltip", "Build, cook and package your game for the {0} platform"), Platform->DisplayName()),
			FSlateIcon(FEditorStyle::GetStyleSetName(), *FString::Printf(TEXT("Launcher.Platform_%s"), *Platform->PlatformName())),
			Action
			);
	}


	/**
	* Creates the platform menu entries for a given platforms sub-platforms.
	* e.g. Windows has multiple sub-platforms - Win32 and Win64
	*
	* @param MenuBuilder	- The builder for the menu that owns this menu.
	* @param Platform		- The target platform we allow packaging for
	* @param DisplayNames	- The Sub-platform display names
	*/
	static void AddPlatformSubPlatformsToMenu(FMenuBuilder& MenuBuilder, const ITargetPlatform* Platform, TArray<FText> DisplayNames)
	{
		for (const FText& DisplayName : DisplayNames)
		{
			FString OptionalParams = FString::Printf(TEXT("-targetplatform=%s"), *DisplayName.ToString());
			FUIAction Action(
				FExecuteAction::CreateStatic(&FMainFrameActionCallbacks::PackageProject, Platform->PlatformName(), DisplayName, OptionalParams),
				FCanExecuteAction::CreateStatic(&FMainFrameActionCallbacks::PackageProjectCanExecute, Platform->PlatformName(), Platform->SupportsFeature(ETargetPlatformFeatures::Packaging))
				);

			MenuBuilder.AddMenuEntry(
				DisplayName,
				FText::Format(LOCTEXT("PackageGameForPlatformTooltip", "Build, cook and package your game for the {0} | {1} platform"), Platform->DisplayName(), DisplayName),
				FSlateIcon(FEditorStyle::GetStyleSetName(), *FString::Printf(TEXT("Launcher.Platform_%s"), *Platform->PlatformName())),
				Action
				);
		}
	}

	/**
	* Creates a build configuration sub-menu.
	*
	* @param MenuBuilder - The builder for the menu that owns this menu.
	*/
	static void MakeBuildConfigurationsMenu(FMenuBuilder& MenuBuilder)
	{
		// Only show the debug game option if the game has source code. 
		TArray<FString> TargetFileNames;
		IFileManager::Get().FindFiles(TargetFileNames, *(FPaths::GameSourceDir() / TEXT("*.target.cs")), true, false);

		if (TargetFileNames.Num() > 0)
		{
			MenuBuilder.AddMenuEntry(
				LOCTEXT("DebugConfiguration", "DebugGame"),
				LOCTEXT("DebugConfigurationTooltip", "Package the project for debugging"),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateStatic(&FMainFrameActionCallbacks::PackageBuildConfiguration, PPBC_DebugGame),
					FCanExecuteAction(),
					FIsActionChecked::CreateStatic(&FMainFrameActionCallbacks::PackageBuildConfigurationIsChecked, PPBC_DebugGame)
				),
				NAME_None,
				EUserInterfaceActionType::RadioButton
			);
		}

		MenuBuilder.AddMenuEntry(
			LOCTEXT("DevelopmentConfiguration", "Development"),
			LOCTEXT("DevelopmentConfigurationTooltip", "Package the project for development"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateStatic(&FMainFrameActionCallbacks::PackageBuildConfiguration, PPBC_Development),
				FCanExecuteAction(),
				FIsActionChecked::CreateStatic(&FMainFrameActionCallbacks::PackageBuildConfigurationIsChecked, PPBC_Development)
			),
			NAME_None,
			EUserInterfaceActionType::RadioButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("ShippingConfiguration", "Shipping"),
			LOCTEXT("ShippingConfigurationTooltip", "Package the project for shipping"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateStatic(&FMainFrameActionCallbacks::PackageBuildConfiguration, PPBC_Shipping),
				FCanExecuteAction(),
				FIsActionChecked::CreateStatic(&FMainFrameActionCallbacks::PackageBuildConfigurationIsChecked, PPBC_Shipping)
			),
			NAME_None,
			EUserInterfaceActionType::RadioButton
		);
	}
};


#undef LOCTEXT_NAMESPACE