// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Settings : ModuleRules
{
	public Settings(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
			}
		);

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );

		PrivateIncludePaths.AddRange(
			new string[]
            {
                "Developer/Settings/Private",
                "Developer/Settings/Private/Widgets",
            }
		);
	}
}
