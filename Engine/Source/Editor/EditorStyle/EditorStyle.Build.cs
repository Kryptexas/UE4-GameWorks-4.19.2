// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class EditorStyle : ModuleRules
{
    public EditorStyle(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Slate"
			}
		);

		PrivateDependencyModuleNames.Add("TargetPlatform");

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"Settings",
			}
		);

		OptimizeCode = CodeOptimization.Never;
	}
}
