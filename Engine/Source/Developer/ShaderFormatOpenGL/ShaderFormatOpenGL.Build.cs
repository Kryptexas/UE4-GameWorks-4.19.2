// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ShaderFormatOpenGL : ModuleRules
{
	public ShaderFormatOpenGL(TargetInfo Target)
	{

		PrivateIncludePathModuleNames.Add("TargetPlatform");
		PrivateIncludePathModuleNames.Add("OpenGLDrv"); 

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"ShaderCore",
				"ShaderPreprocessor"
			}
			);

		AddThirdPartyPrivateStaticDependencies(Target, 
			"OpenGL",
			"HLSLCC"
			);
	}
}
