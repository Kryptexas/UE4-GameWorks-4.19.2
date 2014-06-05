// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ShaderCompilerCommon : ModuleRules
{
	public ShaderCompilerCommon(TargetInfo Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"ShaderCore",
			}
			);
	}
}
