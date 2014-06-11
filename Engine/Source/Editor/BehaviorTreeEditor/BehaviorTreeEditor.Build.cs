// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BehaviorTreeEditor : ModuleRules
{
	public BehaviorTreeEditor(TargetInfo Target)
	{
        PrivateIncludePaths.AddRange(
            new string[] {
				"Editor/GraphEditor/Private",
				"Editor/Kismet/Private",
				"Editor/BehaviorTreeEditor/Private",
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
				"Engine", 
                "RenderCore",
                "InputCore",
				"Slate",
				"SlateCore",
                "EditorStyle",
				"UnrealEd", 
				"MessageLog", 
				"GraphEditor",
                "Kismet",
                "PropertyEditor",
				"AnimGraph",
				"BlueprintGraph",
                "AIModule",
                "GameplayDebugger",
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
