// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class ShaderCompileWorker : ModuleRules
{
	public ShaderCompileWorker(TargetInfo Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"Projects",
				"ShaderCore",
				"SandboxFile",
				"TargetPlatform",
			}
			);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"Launch",
				"TargetPlatform",
			}
			);

		PrivateIncludePaths.Add("Runtime/Launch/Private");		// For LaunchEngineLoop.cpp include
	}
}

