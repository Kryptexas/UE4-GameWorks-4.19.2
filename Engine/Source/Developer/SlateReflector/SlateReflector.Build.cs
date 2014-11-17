// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SlateReflector : ModuleRules
{
	public SlateReflector(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"Slate",
				"SlateCore",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"InputCore",
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				"Developer/SlateReflector/Private",
				"Developer/SlateReflector/Private/Models",
				"Developer/SlateReflector/Private/Widgets",
			}
		);
	}
}
