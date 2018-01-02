// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ModuleUI : ModuleRules
{
	public ModuleUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
                "InputCore",
				"Projects",
				"Slate",
				"SlateCore",
				"Engine"
			}
		);
	}
}
