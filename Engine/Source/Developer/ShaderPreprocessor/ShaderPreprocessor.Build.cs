// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ShaderPreprocessor : ModuleRules
{
	public ShaderPreprocessor(TargetInfo Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"ShaderCore",
			}
			);

		AddEngineThirdPartyPrivateStaticDependencies(Target, "MCPP");
	}
}
