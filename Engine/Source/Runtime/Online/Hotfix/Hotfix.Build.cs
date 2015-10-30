// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Hotfix : ModuleRules
{
	public Hotfix(TargetInfo Target)
    {
        PrivateDependencyModuleNames.AddRange(
			new string[] { 
				"Core",
				"CoreUObject",
				"OnlineSubsystem",
			}
			);
    }
}
