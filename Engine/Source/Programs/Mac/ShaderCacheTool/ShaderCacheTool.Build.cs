// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ShaderCacheTool : ModuleRules
{
	public ShaderCacheTool(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicIncludePaths.Add("Runtime/Launch/Public");

		PrivateIncludePaths.Add("Runtime/Launch/Private");		// For LaunchEngineLoop.cpp include

		PrivateDependencyModuleNames.AddRange(new string []
			{
				"Core",
				"ApplicationCore",
				"Projects",
				"ShaderCore",
			});
	}
}
