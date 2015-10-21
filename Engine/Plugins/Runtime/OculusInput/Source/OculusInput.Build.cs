// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class OculusInput : ModuleRules
	{
		public OculusInput(TargetInfo Target)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"InputCore",
					"Engine",
				}
				);

			PublicIncludePathModuleNames.AddRange( new string[] {"OculusRift"} );

			PrivateIncludePathModuleNames.AddRange(
				new string[]
				{
					"OculusRift",			// For IOculusRiftPlugin.h
                    "InputDevice",			// For IInputDevice.h
					"HeadMountedDisplay"	// For IMotionController.h
				}
				);

            // Currently, the Rift is only supported on windows and mac platforms
            if (Target.Platform == UnrealTargetPlatform.Win32 || Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Mac)
            {
//				Definitions.Add("USE_OVR_MOTION_SDK=" + ( UEBuildConfiguration.bOculusInputControllerSupport ? "1" : "0" ) );
				Definitions.Add("USE_OVR_MOTION_SDK=" + ( true ? "1" : "0" ) );
				
				PrivateDependencyModuleNames.AddRange(new string[] { "LibOVR" });
            }
		}
	}
}