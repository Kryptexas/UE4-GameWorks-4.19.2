// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MainFrame : ModuleRules
{
	public MainFrame(ReadOnlyTargetRules Target) : base(Target)
	{

        PublicDependencyModuleNames.AddRange(
            new string[]
			{
                "Documentation",
			}
        );

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"EngineSettings",
                "InputCore",
				"RHI",
				"ShaderCore",
				"Slate",
				"SlateCore",
                "EditorStyle",
				"SourceControl",
				"SourceControlWindows",
				"TargetPlatform",
				"DesktopPlatform",
				"UnrealEd",
				"WorkspaceMenuStructure",
				"MessageLog",
				"UATHelper",
				"TranslationEditor",
				"Projects",
                "DeviceProfileEditor",
                "UndoHistory",
				"Analytics",
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"AssetTools",
				"DesktopPlatform",
				"GameProjectGeneration",
				"ProjectTargetPlatformEditor",
				"LevelEditor",
                "OutputLog",
				"SuperSearch",
				"Settings",
				"SourceCodeAccess",
                "Toolbox",
                "LocalizationDashboard",
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
                "Editor/MainFrame/Private",
				"Editor/MainFrame/Private/Frame",
                "Editor/MainFrame/Private/Menus",
            }
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"AssetTools",
				"GameProjectGeneration",
				"ProjectTargetPlatformEditor",
				"LevelEditor",
                "OutputLog",
				"SuperSearch",
				"SourceCodeAccess",
				"HotReload",
                "LocalizationDashboard",
			}
		);
	}
}
