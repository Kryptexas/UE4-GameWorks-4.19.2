// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FoliageEdit : ModuleRules
{
	public FoliageEdit(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"InputCore",
				"Engine",
				"UnrealEd",
				"Slate",
                "EditorStyle",
				"RenderCore",
				"LevelEditor"
			}
			);
	}
}
