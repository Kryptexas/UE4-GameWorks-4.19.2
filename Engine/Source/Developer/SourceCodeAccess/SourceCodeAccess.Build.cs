// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SourceCodeAccess : ModuleRules
{
	public SourceCodeAccess(TargetInfo Target)
	{
        PrivateIncludePaths.Add("Developer/SourceCodeAccess/Private");

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Settings",
				"Slate"
			}
			);
	}
}
