// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SCWEditor : ModuleRules
{
	public SCWEditor(TargetInfo Target)
	{
		PrivateIncludePaths.Add("SlateComponentWrapperEditor/Private");

		PrivateDependencyModuleNames.AddRange(new string[] {
				"Core",
				"CoreUObject",
				"Slate",
				"Engine",
				"AssetTools",
				"UnrealEd",
				"KismetWidgets",
				"GraphEditor",
				"Kismet",
				"PropertyEditor",
				"ContentBrowser",
				"WorkspaceMenuStructure",
				"SCWRuntime"
			});
	}
}
