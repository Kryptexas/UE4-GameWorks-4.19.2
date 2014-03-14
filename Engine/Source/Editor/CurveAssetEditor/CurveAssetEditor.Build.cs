// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CurveAssetEditor : ModuleRules
{
	public CurveAssetEditor(TargetInfo Target)
	{
		PublicIncludePathModuleNames.Add("LevelEditor");
        PublicIncludePathModuleNames.Add("WorkspaceMenuStructure");

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
                "EditorStyle",
				"UnrealEd"
			}
			);

		DynamicallyLoadedModuleNames.Add("WorkspaceMenuStructure");
	}
}
