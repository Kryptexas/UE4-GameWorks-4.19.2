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
                "EditorStyle",
				"SourceControl",
				"SourceControlWindows",
				"TargetPlatform",
				"UnrealEd",
				"WorkspaceMenuStructure",
				"MessageLog",
//				"SearchUI",
				"Settings",
				"TranslationEditor"
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"AssetTools",
				"DesktopPlatform",
				"GameProjectGeneration",
				"LevelEditor",
				"OutputLog",
				"VSAccessor",
			}
		);

		PrivateIncludePaths.AddRange(
			new string[]
            {
                "Editor/MainFrame/Private",
				"Editor/MainFrame/Private/Frame",
                "Editor/MainFrame/Private/Menus",
            }
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				"AssetTools",
				"DesktopPlatform",
                "Documentation",
				"GameProjectGeneration",
				"LevelEditor",
				"OutputLog",
                "TranslationEditor",
			}
		);


		if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
		{
			DynamicallyLoadedModuleNames.Add("VSAccessor");
		}
	}
}
