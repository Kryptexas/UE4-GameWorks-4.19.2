// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SmartSnapping : ModuleRules
{
	public SmartSnapping(TargetInfo Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"UnrealEd",
				"Slate",
				"LevelEditor",
				"ViewportSnapping"
			});
	}
}
