// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class OculusRift : ModuleRules
	{
		public OculusRift(TargetInfo Target)
		{
			PrivateIncludePaths.AddRange(
				new string[] {
					"OculusRift/Private",
					"../../../Source/Runtime/Renderer/Private",
					// ... add other private include paths required here ...
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"RHI",
					"RenderCore",
					"Renderer",
					"ShaderCore",
					"HeadMountedDisplay"
				}
				);

            // Currently, the Rift is only supported on windows platforms
            if (Target.Platform == UnrealTargetPlatform.Win32 || Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Mac)
            {
				PrivateDependencyModuleNames.AddRange(new string[] { "LibOVR" });
            }
		}
	}
}