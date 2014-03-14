// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LevelEditor : ModuleRules
{
	public LevelEditor(TargetInfo Target)
	{
		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"AssetTools",
				"Kismet",
				"MainFrame",
                "PlacementMode"
			}
		);

		PublicIncludePathModuleNames.AddRange(
			new string[] {
				"UserFeedback"
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Analytics",
				"Core",
				"CoreUObject",
				"DesktopPlatform",
                "InputCore",
				"Slate",
                "EditorStyle",
				"Engine",
				"MessageLog",
				"Settings",
                "SourceControl",
                "StatsViewer",
				"UnrealEd", 
				"RenderCore",
				"DeviceProfileServices"
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"MainFrame",
				"PropertyEditor",
				"SceneOutliner",
				"ClassViewer",
				"ContentBrowser",
				"DeviceManager",
				"SettingsEditor",
				"SessionFrontend",
				"AutomationWindow",
				"Layers",
				"Levels",
                "WorldBrowser",
				"TaskBrowser",
				"EditorWidgets",
				"AssetTools",
				"WorkspaceMenuStructure",
				"NewLevelDialog",
				"DeviceProfileEditor",
				"DeviceProfileServices",
                "PlacementMode",
				"UserFeedback"
			}
		);
	}
}
