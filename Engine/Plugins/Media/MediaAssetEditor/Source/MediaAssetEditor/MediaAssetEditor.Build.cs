// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MediaAssetEditor : ModuleRules
{
	public MediaAssetEditor(TargetInfo Target)
	{
		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
                "AssetTools",
				"MainFrame",
                "Media",
				"WorkspaceMenuStructure",
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				"MediaAssetEditor/Private",
                "MediaAssetEditor/Private/AssetTools",
                "MediaAssetEditor/Private/Factories",
                "MediaAssetEditor/Private/Models",
                "MediaAssetEditor/Private/Styles",
                "MediaAssetEditor/Private/Widgets",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
                "ContentBrowser",
				"Core",
				"CoreUObject",
                "EditorStyle",
				"Engine",
                "InputCore",
                "MediaAssets",
                "PropertyEditor",
				"RenderCore",
				"RHI",
				"ShaderCore",
				"Slate",
				"SlateCore",
                "TextureEditor",
				"UnrealEd"
			}
		);

        PrivateIncludePathModuleNames.AddRange(
            new string[] {
                "AssetTools",
                "Media",
				"UnrealEd",
                "WorkspaceMenuStructure",
			}
        );
    }
}
