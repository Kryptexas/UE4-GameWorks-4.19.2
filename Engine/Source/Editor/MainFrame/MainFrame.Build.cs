// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MainFrame : ModuleRules
{
	public MainFrame(TargetInfo Target)
	{

        PublicDependencyModuleNames.AddRange(
            new string[]
			{
                "Documentation"
			}
        );

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"CrashTracker",
				"Engine",
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
//				"SearchUI",
				"TranslationEditor",
				"Projects",
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
				"Settings",
				"SourceCodeAccess",
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
				"DesktopPlatform",
                "Documentation",
				"GameProjectGeneration",
				"ProjectTargetPlatformEditor",
				"LevelEditor",
				"OutputLog",
                "TranslationEditor",
				"SourceCodeAccess",
			}
		);
	}
}
