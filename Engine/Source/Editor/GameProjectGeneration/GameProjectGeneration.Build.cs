// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GameProjectGeneration : ModuleRules
{
    public GameProjectGeneration(TargetInfo Target)
	{
        PrivateIncludePathModuleNames.AddRange(
            new string[] {
				"ClassViewer",
                "DesktopPlatform",
                "MainFrame",
            }
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Analytics",
				"Core",
				"CoreUObject",
				"Engine",
				"EngineSettings",
                "InputCore",
				"Projects",
                "RenderCore",
				"Slate",
                "EditorStyle",
                "SourceControl",
                "UnrealEd",
			}
		);

        DynamicallyLoadedModuleNames.AddRange(
            new string[] {
				"ClassViewer",
                "DesktopPlatform",
                "MainFrame",
            }
		);
	}
}
