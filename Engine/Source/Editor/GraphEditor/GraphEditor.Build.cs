// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GraphEditor : ModuleRules
{
	public GraphEditor(TargetInfo Target)
	{
		PrivateIncludePaths.AddRange(
			new string[] {
				"Editor/GraphEditor/Private",
				"Editor/GraphEditor/Private/AnimationPins",
				"Editor/GraphEditor/Private/AnimationStateNodes",
				"Editor/GraphEditor/Private/KismetNodes",
				"Editor/GraphEditor/Private/KismetPins",
				"Editor/GraphEditor/Private/MaterialNodes",
				"Editor/GraphEditor/Private/MaterialPins",
				"Editor/GraphEditor/Private/NoRedist",
				"Editor/GraphEditor/Private/SoundNodes",
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[] { 
				"EditorWidgets"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
                "InputCore",
				"Slate",
                "EditorStyle",
				"UnrealEd",
				"AssetRegistry",
				"KismetWidgets",
				"BlueprintGraph",
				"AnimGraph",
				"Documentation",
				"RenderCore",
				"RHI"
			}
			);

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"ContentBrowser",
				"Documentation",
				"EditorWidgets"
			}
			);
	}
}
