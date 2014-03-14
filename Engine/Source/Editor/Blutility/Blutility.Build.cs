// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Blutility : ModuleRules
{
	public Blutility(TargetInfo Target)
	{
		PrivateIncludePaths.Add("Editor/Blutility/Private");

		PrivateIncludePathModuleNames.Add("AssetTools");

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
                "InputCore",
				"Slate",
                "EditorStyle",
				"UnrealEd",
				"Kismet",
				"AssetRegistry",
				"AssetTools",
				"WorkspaceMenuStructure",
				"ContentBrowser",
				"ClassViewer",
				"CollectionManager",
                "PropertyEditor"
			}
			);
	}
}
