// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MovieSceneTools : ModuleRules
{
	public MovieSceneTools(TargetInfo Target)
	{
		PrivateIncludePaths.Add("Developer/MovieSceneTools/Private");

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
                "InputCore",
				"UnrealEd"
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"MovieSceneCore",
				"MovieSceneCoreTypes",
				"BlueprintGraph",
				"Slate",
                "EditorStyle",
				"RenderCore",
				"RHI"
			}
			);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"AssetTools",
				"Sequencer",
				"PropertyEditor"
			}
			);

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"AssetTools",
				"Sequencer",
				"PropertyEditor"
			}
			);

	}
}
