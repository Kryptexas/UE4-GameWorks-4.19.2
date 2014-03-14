// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class StandaloneRenderer : ModuleRules
{
	public StandaloneRenderer(TargetInfo Target)
	{
		PrivateIncludePaths.Add("Developer/StandaloneRenderer/Private");

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"ImageWrapper",
				"InputCore",
				"Slate",
			}
			);

		AddThirdPartyPrivateStaticDependencies(Target, "OpenGL");

		if ((Target.Platform == UnrealTargetPlatform.Win64) ||
			(Target.Platform == UnrealTargetPlatform.Win32))
		{
			// @todo: This should be private? Not sure!!
			AddThirdPartyPrivateStaticDependencies(Target, "DX11");
		}

		if (Target.Platform == UnrealTargetPlatform.IOS)
		{
			PublicFrameworks.AddRange(new string[] { "OpenGLES", "QuartzCore", "GLKit" });
		}
	}
}
