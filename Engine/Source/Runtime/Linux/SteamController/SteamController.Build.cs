// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SteamController : ModuleRules
{
    public SteamController(TargetInfo Target)
    {
        PrivateIncludePathModuleNames.Add("TargetPlatform");

        Definitions.Add("WITH_STEAMCONTROLLER=1");
        PrivateDependencyModuleNames.AddRange(new string[]
        {
			"Core",
			"CoreUObject",
			"Engine",
		});

        AddThirdPartyPrivateStaticDependencies(Target,
            "Steamworks"
        );
    }
}
