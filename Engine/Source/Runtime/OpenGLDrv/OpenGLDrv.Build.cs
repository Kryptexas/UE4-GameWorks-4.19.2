// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class OpenGLDrv : ModuleRules
{
	public OpenGLDrv(TargetInfo Target)
	{
		PrivateIncludePaths.Add("Runtime/OpenGLDrv/Private");

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core", 
				"CoreUObject", 
				"Engine", 
				"RHI", 
				"RenderCore", 
				"ShaderCore", 
			}
			);

		PrivateIncludePathModuleNames.Add("ImageWrapper");
		DynamicallyLoadedModuleNames.Add("ImageWrapper");

		AddThirdPartyPrivateStaticDependencies(Target, "OpenGL");

		if (Target.Platform == UnrealTargetPlatform.HTML5 && Target.Architecture == "-win32")
		{
			AddThirdPartyPrivateStaticDependencies(Target, "SDL");
			AddThirdPartyPrivateStaticDependencies(Target, "ANGLE");
		}

		if (Target.Configuration != UnrealTargetConfiguration.Shipping)
		{
			PrivateIncludePathModuleNames.AddRange(
				new string[]
				{
					"TaskGraph"
                }
			);
		}
	}
}
