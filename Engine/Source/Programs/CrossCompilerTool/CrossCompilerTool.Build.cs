// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CrossCompilerTool : ModuleRules
{
	public CrossCompilerTool(TargetInfo Target)
	{
		PublicIncludePaths.Add("Runtime/Launch/Public");

		PrivateIncludePaths.Add("Runtime/Launch/Private");		// For LaunchEngineLoop.cpp include
		PrivateIncludePaths.Add("Developer/iOS/MetalShaderFormat/Private");		// For Metal includes
		PrivateIncludePaths.Add("Developer/ShaderFormatOpenGL/Private");		// For GLSL includes

		PrivateDependencyModuleNames.AddRange(new string []
			{
				"Core",
				"Projects",
				"MetalShaderFormat",
				"ShaderFormatOpenGL",
				"ShaderCompilerCommon",
			});

		AddThirdPartyPrivateStaticDependencies(Target,
			"HLSLCC"
		);
	}
}
