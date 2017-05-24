// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System.IO;

namespace UnrealBuildTool.Rules
{
	public class GearVRController : ModuleRules
	{
		public GearVRController(ReadOnlyTargetRules Target) : base(Target)
        {
			PrivateIncludePaths.AddRange(new string[]
				{
					"GearVR",
                    "../../../../Source/Runtime/Renderer/Private",
                    "../../../../Source/Runtime/Launch/Private",
                    "../../../../Source/ThirdParty/Oculus/Common",
                    "../../../../Source/Runtime/OpenGLDrv/Private",
                });

			PrivateDependencyModuleNames.AddRange(new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"InputCore",
                    "InputDevice",
                    "GearVR",
					"HeadMountedDisplay",
					"OculusMobile",
                    "UtilityShaders",
                    "OpenGLDrv",
                    "ShaderCore",
                    "RenderCore"
                });

            if (UEBuildConfiguration.bBuildEditor)
            {
                PrivateDependencyModuleNames.Add("UnrealEd");
            }

            if (Target.Platform == UnrealTargetPlatform.Android)
			{
				string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, BuildConfiguration.RelativeEnginePath);
				AdditionalPropertiesForReceipt.Add(new ReceiptProperty("AndroidPlugin", Path.Combine(PluginPath, "GearVRController_APL.xml")));
			}

			PublicIncludePathModuleNames.Add("Launch");

			AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenGL");
		}
	}
}
