// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class ComposureEditor : ModuleRules
	{
		public ComposureEditor(ReadOnlyTargetRules Target) : base(Target)
		{
			PrivateIncludePaths.AddRange(new string[] {
				"ComposureEditorEditor/Private",
			});

			PublicDependencyModuleNames.AddRange(new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
			});

			PrivateDependencyModuleNames.AddRange(new string[] {
				"Composure",
				"MovieScene",
				"MovieSceneTracks",
				"MovieSceneTools",
				"Sequencer",
				"Slate",
				"SlateCore",
				"DesktopWidgets",
				"EditorStyle",
				"UnrealEd"
			});
		}
	}
}
