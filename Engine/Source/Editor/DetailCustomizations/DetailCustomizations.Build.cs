// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DetailCustomizations : ModuleRules
{
	public DetailCustomizations(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePaths.Add("Editor/DetailCustomizations/Private");	// For PCH includes (because they don't work with relative paths, yet)

		PrivateDependencyModuleNames.AddRange(
			new string[] {
                "AppFramework",
				"Core",
				"CoreUObject",
				"ApplicationCore",
				"DesktopWidgets",
				"Engine",
				"Landscape",
                "InputCore",
				"Slate",
				"SlateCore",
                "EditorStyle",
				"UnrealEd",
				"EditorWidgets",
				"KismetWidgets",
				"MovieSceneCapture",
				"MovieSceneTools",
				"MovieSceneTracks",
				"MovieScene",
				"SharedSettingsWidgets",
                "ContentBrowser",
				"BlueprintGraph",
                "GraphEditor",
				"AnimGraph",
                "PropertyEditor",
                "LevelEditor",
				"DesktopPlatform",
				"ClassViewer",
				"TargetPlatform",
				"ExternalImagePicker",
				"MoviePlayer",
				"SourceControl",
                "InternationalizationSettings",
				"SourceCodeAccess",
				"RHI",
                "HardwareTargeting",
                "AIModule", 
				"ConfigEditor",
                "Persona",
                "CinematicCamera",
				"ComponentVisualizers",
                "SkeletonEditor",
                "LevelSequence",
                "AdvancedPreviewScene",
                "AudioSettingsEditor",
				"HeadMountedDisplay",
            }
		);

        PrivateIncludePathModuleNames.AddRange(
			new string[] {
                "Engine",
                "Media",
				"Landscape",
				"LandscapeEditor",
				"PropertyEditor",
				"GameProjectGeneration",
                "ComponentVisualizers",
				"GraphEditor",
                "MeshMergeUtilities",
                "MeshReductionInterface",
            }
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"Layers",
				"GameProjectGeneration",
                "MeshMergeUtilities",
                "MeshReductionInterface",
            }
		);

		// @third party code - BEGIN HairWorks
		AddEngineThirdPartyPrivateStaticDependencies(Target, "HairWorks");
		// @third party code - END HairWorks
	}
}
