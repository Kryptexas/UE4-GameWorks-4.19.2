// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DetailCustomizations : ModuleRules
{
	public DetailCustomizations(TargetInfo Target)
	{
		PrivateIncludePaths.Add("Editor/DetailCustomizations/Private");	// For PCH includes (because they don't work with relative paths, yet)

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
                "InputCore",
				"Slate",
				"SlateCore",
                "EditorStyle",
				"UnrealEd",
				"EditorWidgets",
				"KismetWidgets",
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
				"GameProjectGeneration",
				"MoviePlayer",
				"SourceControl",
                "InternationalizationSettings",
				"SourceCodeAccess",
				"RHI"
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"PropertyEditor",
				"LandscapeEditor",
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"PropertyEditor",
				"Layers"
			}
		);
	}
}
