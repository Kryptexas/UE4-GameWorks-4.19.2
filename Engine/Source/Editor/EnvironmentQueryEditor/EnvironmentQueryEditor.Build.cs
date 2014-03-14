// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class EnvironmentQueryEditor : ModuleRules
{
	public EnvironmentQueryEditor(TargetInfo Target)
	{
        PrivateIncludePaths.AddRange(
            new string[] {
				"Editor/GraphEditor/Private"
			}
            );

        PrivateIncludePathModuleNames.AddRange(
            new string[] {
				"AssetRegistry",
				"AssetTools",
                "PropertyEditor"
			}
            );

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core", 
				"CoreUObject", 
                "InputCore",
				"Engine", 
                "RenderCore",
				"Slate", 
                "EditorStyle",
				"UnrealEd", 
				"MessageLog", 
				"GraphEditor",
                "PropertyEditor",
				"AnimGraph",
				"BlueprintGraph"
			}
			);

		PublicIncludePathModuleNames.Add("LevelEditor");

        DynamicallyLoadedModuleNames.AddRange(
            new string[] { 
                "WorkspaceMenuStructure",
                "PropertyEditor"
            }
            );

	}
}
