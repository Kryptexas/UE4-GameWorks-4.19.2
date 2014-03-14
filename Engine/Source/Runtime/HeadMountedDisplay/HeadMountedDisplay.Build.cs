// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class HeadMountedDisplay : ModuleRules
{
    public HeadMountedDisplay(TargetInfo Target)
	{
		PublicIncludePaths.AddRange(
			new string[] {
				"Runtime/HeadMountedDisplay/Public"
			}
			);
		PrivateDependencyModuleNames.AddRange( new string[] { "Core", "CoreUObject", "Engine", "Slate" } );

        PCHUsage = PCHUsageMode.NoSharedPCHs;
	}
}
