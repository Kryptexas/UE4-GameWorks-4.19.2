// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class PhysXFormats : ModuleRules
{
	public PhysXFormats(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePathModuleNames.Add("TargetPlatform");

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject", // @todo Mac: for some reason it's needed to link in debug on Mac
				"Engine"
			}
			);

        PrivateIncludePaths.AddRange(
            new string[] {
                "Runtime/Engine/Private",   //Not sure why this is its own format. The two modules depend on eachother.
			}
        );

		SetupModulePhysXAPEXSupport(Target);
	}
}
