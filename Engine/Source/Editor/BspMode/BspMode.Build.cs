// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BspMode : ModuleRules
{
	public BspMode(TargetInfo Target)
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
				"LevelEditor"
			}
			);

        DynamicallyLoadedModuleNames.AddRange(
            new string[] {
				"PropertyEditor",
                "LevelEditor"
			}
        );
	}
}
