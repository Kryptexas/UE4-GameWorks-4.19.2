// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UMGEditor : ModuleRules
{
	public UMGEditor(TargetInfo Target)
	{
		PrivateIncludePaths.AddRange(
			new string[] {
				"Editor/GraphEditor/Private",
				"Editor/Kismet/Private",
				"Developer/AssetTools/Private/AssetTypeActions",
				"Editor/UMGEditor/Private", // For PCH includes (because they don't work with relative paths, yet)
				"Editor/UMGEditor/Private/Templates",
                "Editor/UMGEditor/Private/Extensions",
			});

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"AssetTools",
				"UMG",
			});

		PublicIncludePaths.AddRange(
			new string[] {
				"Editor/DistCurveEditor/Public",
				"Editor/UnrealEd/Public",
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Slate",
				"Engine",
				"InputCore",
				"AssetTools",
				"UnrealEd", // for FAssetEditorManager
				"KismetWidgets",
				"KismetCompiler",
				"BlueprintGraph",
				"GraphEditor",
				"Kismet",  // for FWorkflowCentricApplication
				"PropertyEditor",
				"RenderCore",
				"LevelEditor", // for EdModes to get a toolkit host  //@TODO: PAPER: Should be a better way to do this (see the @todo in EdModeTileMap.cpp)
				"UMG",
				"DistCurveEditor",
				"ContentBrowser",
				"WorkspaceMenuStructure",
				"EditorStyle",
				"Slate",
				"SlateCore",
			}
			);

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"MainFrame",
				"PropertyEditor",
				"AssetTools",
				"UMG",
			}
			);
	}
}
