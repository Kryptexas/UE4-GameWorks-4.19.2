// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TextureEditor : ModuleRules
{
	public TextureEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"MainFrame",
				"WorkspaceMenuStructure"
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
                "PropertyEditor"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
                "AppFramework",
				"Core",
				"CoreUObject",
                "InputCore",
				"Engine",
				"RenderCore",
				"ShaderCore",
				"RHI",
				"Slate",
				"SlateCore",
                "EditorStyle",
				"UnrealEd",
                "PropertyEditor"
            }
		);
	}
}
