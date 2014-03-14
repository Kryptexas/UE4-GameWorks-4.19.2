// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ShaderCore : ModuleRules
{
	public ShaderCore(TargetInfo Target)
	{
		PublicDependencyModuleNames.Add("RHI");

		PrivateDependencyModuleNames.AddRange(new string[] { "Core", "RenderCore" });

		PrivateIncludePathModuleNames.AddRange(new string[] { "DerivedDataCache", "TargetPlatform" });
	}
}