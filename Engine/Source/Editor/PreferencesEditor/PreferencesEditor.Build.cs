// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PreferencesEditor : ModuleRules
{
	public PreferencesEditor(TargetInfo Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
                "InputCore",
				"Slate",
                "EditorStyle",
				"UnrealEd",
				"SourceControl",
                "PropertyEditor",
                "DesktopPlatform",
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[]
			{
				"MainFrame",
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				"MainFrame",
			}
		);
	}
}
