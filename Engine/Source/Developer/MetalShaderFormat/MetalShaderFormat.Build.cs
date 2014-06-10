// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MetalShaderFormat : ModuleRules
{
	public MetalShaderFormat(TargetInfo Target)
	{
		PrivateIncludePathModuleNames.Add("TargetPlatform");

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"ShaderCore",
				"ShaderPreprocessor"
			}
			);

		AddThirdPartyPrivateStaticDependencies(Target, 
			"HLSLCC"
			);
	}
}
