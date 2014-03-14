// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SoundCueEditor : ModuleRules
{
	public SoundCueEditor(TargetInfo Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
                "EditorStyle",
				"UnrealEd",
				"GraphEditor",
			}
		   );

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"PropertyEditor",
				"WorkspaceMenuStructure"
			}
			);
	}
}
