// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class NewLevelDialog : ModuleRules
{
	public NewLevelDialog(TargetInfo Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core", 
				"CoreUObject", 
                "InputCore",
				"RenderCore", 
				"Engine", 
				"Slate", 
                "EditorStyle",
				"UnrealEd"
			}
			);
	}
}
