// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class VulkanShaderFormat : ModuleRules
{
	public VulkanShaderFormat(TargetInfo Target)
	{
		PrivateIncludePathModuleNames.Add("TargetPlatform");
		PrivateIncludePathModuleNames.Add("VulkanRHI"); 

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"ShaderCore",
				"ShaderCompilerCommon",
				"ShaderPreprocessor",
			}
			);

        AddEngineThirdPartyPrivateStaticDependencies(Target, "HLSLCC");
		AddEngineThirdPartyPrivateStaticDependencies(Target, "GlsLang");
	}
}
