// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CollisionAnalyzer : ModuleRules
{
	public CollisionAnalyzer(TargetInfo Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
                "InputCore",
				"Slate",
                "EditorStyle",
				"Engine",
				"WorkspaceMenuStructure",
			}
			);
	}
}
