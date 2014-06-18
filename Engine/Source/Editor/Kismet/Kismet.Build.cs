// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Kismet : ModuleRules
{
	public Kismet(TargetInfo Target)
	{
		PrivateIncludePaths.Add("Editor/Kismet/Private");

		PrivateIncludePathModuleNames.AddRange(
			new string[] { 
				"AssetRegistry", 
				"AssetTools",
				"EditorWidgets",
				"Analytics"
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Slate",
				"SlateCore",
                "EditorStyle",
				"Engine",
				"MessageLog",
				"UnrealEd",
				"GraphEditor",
				"KismetWidgets",
				"BlueprintGraph",
				"AnimGraph",
				"PropertyEditor",
				"SourceControl",
				"LevelEditor",
                "InputCore",
				"EngineSettings",
                "Projects"
			}
			);

        DynamicallyLoadedModuleNames.AddRange(
            new string[] {
				"ClassViewer",
				"Documentation",
				"EditorWidgets"
			}
            );
	}
}
