// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GeometryMode : ModuleRules
{
	public GeometryMode(TargetInfo Target)
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
				"RenderCore",
				"LevelEditor"
			}
			);
        DynamicallyLoadedModuleNames.AddRange(
            new string[] {
				"PropertyEditor",
                "BspMode"
			}
        );
	}
}
