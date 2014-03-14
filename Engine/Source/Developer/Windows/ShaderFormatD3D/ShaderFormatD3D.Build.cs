// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ShaderFormatD3D : ModuleRules
{
	public ShaderFormatD3D(TargetInfo Target)
	{
		PrivateIncludePathModuleNames.Add("TargetPlatform");

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"ShaderCore",
				"ShaderPreprocessor",
			}
			);

		AddThirdPartyPrivateStaticDependencies(Target, "DX11");
	}
}
