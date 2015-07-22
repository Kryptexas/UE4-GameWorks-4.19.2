// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ActorAnimationEditor : ModuleRules
{
	public ActorAnimationEditor(TargetInfo Target)
	{
        DynamicallyLoadedModuleNames.AddRange(
            new string[] {
				"AssetTools",
			}
        );
        
        PrivateDependencyModuleNames.AddRange(
			new string[] {
                "ActorAnimation",
				"BlueprintGraph",
				"Core",
				"CoreUObject",
                "EditorStyle",
                "Engine",
				"MovieScene",
				"Sequencer",
                "Slate",
                "SlateCore",
                "UnrealEd",
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"AssetTools",
			}
		);

        PrivateIncludePaths.AddRange(
            new string[] {
				"ActorAnimationEditor/Private",
				"ActorAnimationEditor/Private/AssetTools",
				"ActorAnimationEditor/Private/Factories",
				"ActorAnimationEditor/Private/Styles",
			}
        );
	}
}
