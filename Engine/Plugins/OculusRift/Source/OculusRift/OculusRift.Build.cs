// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class OculusRift : ModuleRules
	{
		public OculusRift(TargetInfo Target)
		{
			PrivateIncludePaths.AddRange(
				new string[] {
					"OculusRift/Private",
					// ... add other private include paths required here ...
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
                    "InputCore",
					"Engine",
					"HeadMountedDisplay"
				}
				);

            // Currently, the Rift is only supported on windows platforms
            if (Target.Platform == UnrealTargetPlatform.Win32 || Target.Platform == UnrealTargetPlatform.Win64)
            {
				PrivateDependencyModuleNames.AddRange(new string[] { "LibOVR" });
            }
		}
	}
}