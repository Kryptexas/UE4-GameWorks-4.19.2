// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class OculusLibrary : ModuleRules
	{
		public OculusLibrary(TargetInfo Target)
		{
			PrivateIncludePaths.AddRange(
				new string[] {
					"OculusLibrary/Private",
 					"../../../../Source/Runtime/Renderer/Private",
 					"../../../../Source/ThirdParty/Oculus/Common",
					"../../OculusRift/Source/OculusRift/Private",
					// ... add other private include paths required here ...
				}
				);

			PublicIncludePathModuleNames.Add("OculusRift");

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"InputCore",
					"RHI",
					"RenderCore",
					"Renderer",
					"ShaderCore",
					"HeadMountedDisplay",
					"Slate",
					"SlateCore",
					"UtilityShaders",
 					"ImageWrapper",
				}
				);

			if (UEBuildConfiguration.bBuildEditor == true)
			{
				PrivateDependencyModuleNames.Add("UnrealEd");
			}
            // Currently, the Rift is only supported on windows and mac platforms
            if (Target.Platform == UnrealTargetPlatform.Win32 || Target.Platform == UnrealTargetPlatform.Win64)
            {
                PrivateDependencyModuleNames.AddRange(
                    new string[]
                    {
                        "LibOVR"
                    });
            }
        }

    }
}