// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

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
				"MovieScene",
				"SharedSettingsWidgets",
                "ContentBrowser",
				"BlueprintGraph",
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
				"SharedSettingsWidgets",
                "AIModule", 
                "MeshUtilities",
				"ConfigEditor",
                "Persona",
                "CinematicCamera",
				"ComponentVisualizers",
                "SkeletonEditor",
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
				"GraphEditor"
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"Layers",
				"GameProjectGeneration",
			}
		);
	}
}
