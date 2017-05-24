// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CrashTracker : ModuleRules
{
	public CrashTracker(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePaths.Add("Developer/CrashTracker/Private");

		PublicDependencyModuleNames.Add("Core");

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Engine",
				"ImageWrapper",
				"RenderCore",
				"RHI",
				"ShaderCore",
				"Slate",
				"SlateCore",
			}
		);
	}
}
