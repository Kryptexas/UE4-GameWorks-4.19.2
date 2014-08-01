// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BuildPatchTool : ModuleRules
{
	public BuildPatchTool( TargetInfo Target )
	{
		PublicIncludePaths.Add("Runtime/Launch/Public");

		// For LaunchEngineLoop.cpp include
		PrivateIncludePaths.Add("Runtime/Launch/Private");

		PrivateDependencyModuleNames.AddRange(
			new string[] 
			{
				"Core",
				"BuildPatchServices",
				"Projects",
			}
		);
	}
}