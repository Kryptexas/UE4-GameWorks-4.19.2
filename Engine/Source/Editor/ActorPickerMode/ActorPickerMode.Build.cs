// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ActorPickerMode : ModuleRules
{
    public ActorPickerMode(TargetInfo Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
                "InputCore",
                "Slate",
                "SlateCore",
                "EditorStyle",
				"UnrealEd",
			}
		);
	}
}
