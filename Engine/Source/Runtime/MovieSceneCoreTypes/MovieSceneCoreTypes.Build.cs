// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MovieSceneCoreTypes : ModuleRules
{
	public MovieSceneCoreTypes(TargetInfo Target)
	{
		PrivateIncludePaths.AddRange(new string[]
        { 
            "Runtime/MovieSceneCoreTypes/Private",
            "Runtime/MovieSceneCoreTypes/Private/Instances",
        });

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });

		PrivateDependencyModuleNames.Add("MovieSceneCore");
	}
}
