// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TranslationEditor : ModuleRules
{
	public TranslationEditor(TargetInfo Target)
	{
		PublicIncludePathModuleNames.Add("LevelEditor");
		PublicIncludePathModuleNames.Add("WorkspaceMenuStructure");

        PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"DesktopPlatform",
                "MainFrame",
                "MessageLog",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
                "InputCore",
                "PropertyEditor",
				"Slate",
                "EditorStyle",
				"UnrealEd",
                "GraphEditor",
				"SourceControl",
                "MessageLog",
			}
		);

        PublicDependencyModuleNames.AddRange(
			new string[] {
                "Core",
				"CoreUObject",
				"Engine",
            }
        );


		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"WorkspaceMenuStructure",
				"DesktopPlatform",
				"MainFrame",
				"SourceControl"
			}
		);
	}
}
