// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MovieSceneCapture : ModuleRules
{
	public MovieSceneCapture(TargetInfo Target)
	{
		PrivateIncludePaths.AddRange(
			new string[] {
				"Runtime/MovieSceneCapture/Private"
			}
		);

		var ExternalModules = new string[] {
			"PropertyEditor",
			"ImageWrapper",
		};
		PrivateIncludePathModuleNames.AddRange(ExternalModules);
		DynamicallyLoadedModuleNames.AddRange(ExternalModules);

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"ActorAnimation",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"EditorStyle",
				"Engine",
				"UnrealEd",
				"InputCore",
				"Json",
				"JsonUtilities",
				"MovieScene",
				"Slate",
				"SlateCore",
			}
		);


	}
}
