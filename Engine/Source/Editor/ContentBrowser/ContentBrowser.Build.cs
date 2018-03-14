// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ContentBrowser : ModuleRules
{
	public ContentBrowser(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"AssetRegistry",
				"AssetTools",
				"CollectionManager",
				"EditorWidgets",
				"GameProjectGeneration",
                "MainFrame",
				"PackagesDialog",
				"SourceControl",
				"SourceControlWindows"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
                "AppFramework",
				"Core",
				"CoreUObject",
				"ApplicationCore",
                "InputCore",
				"Engine",
				"Slate",
				"SlateCore",
                "EditorStyle",
				"SourceControl",
				"SourceControlWindows",
				"WorkspaceMenuStructure",
				"UnrealEd",
				"EditorWidgets",
				"Projects",
				"AddContentDialog",
				"DesktopPlatform",
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"PropertyEditor",
				"PackagesDialog",
				"AssetRegistry",
				"AssetTools",
				"CollectionManager",
				"GameProjectGeneration",
                "MainFrame"
			}
		);
		
		PublicIncludePathModuleNames.AddRange(
            new string[] {                
                "IntroTutorials"
            }
        );
	}
}
