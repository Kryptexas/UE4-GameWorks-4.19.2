// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;

public class OpenGL : ModuleRules
{
	public OpenGL(TargetInfo Target)
	{
		Type = ModuleType.External;

		if (Target.Platform != UnrealTargetPlatform.HTML5)
			PublicIncludePaths.Add(UEBuildConfiguration.UEThirdPartyDirectory + "OpenGL");

		if ((Target.Platform == UnrealTargetPlatform.Win64) ||
			(Target.Platform == UnrealTargetPlatform.Win32))
		{
			PublicAdditionalLibraries.Add("opengl32.lib");
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			PublicAdditionalFrameworks.Add("OpenGL");
		}
		else if (Target.Platform == UnrealTargetPlatform.IOS)
		{
			PublicAdditionalFrameworks.Add("OpenGLES");
		}
	}
}
