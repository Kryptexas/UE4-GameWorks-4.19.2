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

		PrivateIncludePathModuleNames.Add("UnrealEd");

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
                "InputCore",
				"Engine",
				"RenderCore",
				"ShaderCore",
				"RHI",
				"Settings",
				"Slate",
                "EditorStyle",
				"UnrealEd"
			}
		);
	}
}
