// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TextureEditor : ModuleRules
{
	public TextureEditor(TargetInfo Target)
	{
		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"MainFrame",
				"WorkspaceMenuStructure",
				"PropertyEditor"
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				"Editor/TextureEditor/Private",
				"Editor/TextureEditor/Private/Menus",
				"Editor/TextureEditor/Private/Models",
				"Editor/TextureEditor/Private/Widgets",
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"Settings",
				"UnrealEd",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
                "InputCore",
				"Engine",
				"RenderCore",
				"ShaderCore",
				"RHI",
				"Slate",
                "EditorStyle",
				"UnrealEd"
			}
		);
	}
}
