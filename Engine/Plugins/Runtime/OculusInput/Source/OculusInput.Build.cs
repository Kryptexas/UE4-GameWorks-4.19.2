// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class OculusInput : ModuleRules
	{
		public OculusInput(TargetInfo Target)
		{
            PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "MediaAssets", "HeadMountedDisplay", "Launch", "RHI", "RenderCore", "Renderer", "OculusRift" });

            PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"InputCore",
					"Engine",
                    "OculusRift",
                    "UtilityShaders",
                }
                );

			PrivateIncludePaths.AddRange(
				new string[] {
					"../../OculusRift/Source/OculusRift/Private",
                    "../../../../Source/Runtime/Renderer/Private",
                     "../../../../Source/ThirdParty/Oculus/Common",
                    "../../../../Source/Runtime/Engine/Classes/Components"
					// ... add other private include paths required here ...
				}
                );

			PublicIncludePathModuleNames.AddRange( new string[] {"OculusRift"} );

			PrivateIncludePathModuleNames.AddRange(
				new string[]
				{
					"OculusRift",			// For IOculusRiftPlugin.h
                    "InputDevice",			// For IInputDevice.h
					"HeadMountedDisplay",	// For IMotionController.h
                    "ImageWrapper",

                }
                );

            // Currently, the Rift is only supported on win64
            if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Win32)
            {	
				PrivateDependencyModuleNames.AddRange(new string[] { "LibOVR" });
            }
		}
	}
}