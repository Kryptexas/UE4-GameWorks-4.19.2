// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Levels : ModuleRules
{
	public Levels(TargetInfo Target)
	{
		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"AssetTools"
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
                "InputCore",
				"Engine",
				"Slate",
                "EditorStyle",
				"UnrealEd",
				"LevelEditor",
				"PropertyEditor",
				"MainFrame",
				"DesktopPlatform",
                "SourceControl",
				"SourceControlWindows",
			}
			);

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"AssetTools",
				"PropertyEditor",
				"SceneOutliner"
			}
			);
	}
}
