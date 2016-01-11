// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System.IO;

namespace UnrealBuildTool.Rules
{
	public class GearVR : ModuleRules
	{
		public GearVR(TargetInfo Target)
		{
			PrivateIncludePaths.AddRange(
				new string[] {
					"GearVR/Private",
					"../../../../Source/Runtime/Renderer/Private",
//					"../../../../Source/Runtime/Launch/Private",
 					"../../../../Source/ThirdParty/Oculus/Common",
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
					"Launch",
					"HeadMountedDisplay"
				}
				);

			PrivateDependencyModuleNames.AddRange(new string[] { "OpenGLDrv" });
			AddThirdPartyPrivateStaticDependencies(Target, "OpenGL");
            PrivateIncludePaths.AddRange(
				new string[] {
					"../../../../Source/Runtime/OpenGLDrv/Private",
					// ... add other private include paths required here ...
					}
				);
			if (Target.Platform == UnrealTargetPlatform.Android)
			{
				PrivateDependencyModuleNames.AddRange(new string[] { "LibOVRMobile" });

				string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, BuildConfiguration.RelativeEnginePath);
				AdditionalPropertiesForReceipt.Add(new ReceiptProperty("AndroidPlugin", Path.Combine(PluginPath, "GearVR_APL.xml")));
			}
		
		}
	}
}
