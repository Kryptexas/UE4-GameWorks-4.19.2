// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BuildPatchServices : ModuleRules
{
	public BuildPatchServices(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);

		PrivateIncludePaths.AddRange(
			new string[]
			{
				"Developer/NoRedist/BuildPatchServices/Private",
			}
		);

		PrivateDependencyModuleNames.AddRange(
		new string[] {
				"HTTP",
				"Analytics",
				"AnalyticsET",
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"HTTP"
			}
		);
	}
}
