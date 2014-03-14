// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SettingsEditor : ModuleRules
{
	public SettingsEditor(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Settings",
				"Slate",
                "EditorStyle",
			}
		);

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
				"Core",
                "CoreUObject",
                "InputCore",
				"DesktopPlatform",
				"PropertyEditor",
				"SourceControl",
            }
        );

		PrivateIncludePaths.AddRange(
			new string[]
            {
                "Developer/SettingsEditor/Private",
				"Developer/SettingsEditor/Private/Models",
                "Developer/SettingsEditor/Private/Widgets",
            }
		);
	}
}
