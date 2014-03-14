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
				"EngineSettings",
                "InputCore",
				"Slate",
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
                "Cascade",
				"DesktopPlatform",
				"BspMode",
				"ClassViewer",
				"GeometryMode"
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
