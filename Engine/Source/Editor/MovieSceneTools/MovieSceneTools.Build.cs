// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MovieSceneTools : ModuleRules
{
	public MovieSceneTools(TargetInfo Target)
	{
		PrivateIncludePaths.AddRange(
            new string[] {
                "Editor/MovieSceneTools/Private",
                "Editor/MovieSceneTools/Private/CurveKeyEditors",
                "Editor/MovieSceneTools/Private/TrackEditors",
            }
        );

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
                "InputCore",
				"UnrealEd",
				"Sequencer"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"MovieScene",
				"MovieSceneTracks",
				"BlueprintGraph",
				"Slate",
				"SlateCore",
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
				"PropertyEditor"
			}
		);
	}
}
