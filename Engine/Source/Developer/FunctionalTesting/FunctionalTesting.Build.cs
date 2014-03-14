// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FunctionalTesting : ModuleRules
{
	public FunctionalTesting(TargetInfo Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"UnrealEd",
                "MessageLog"
			}
			);

		PrivateIncludePaths.AddRange(
			new string[] 
			{
				"MessageLog/Public",
				"Stats/Public",
				"Developer/FunctionalTesting/Private",
			}
			);

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"MessageLog",
			}
			);
	}
}
