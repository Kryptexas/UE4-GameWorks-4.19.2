// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DeviceProfileEditor : ModuleRules
{
	public DeviceProfileEditor(TargetInfo Target)
	{
		PublicIncludePaths.AddRange(
			new string[] {
				"Editor/UnrealEd/Public"
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				"Editor/DeviceProfileEditor/Private",
				"Editor/DeviceProfileEditor/Private/DetailsPanel"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
                "InputCore",
				"Slate",
                "EditorStyle",
				"LevelEditor",
				"UnrealEd",
				"WorkspaceMenuStructure",
				"PropertyEditor",
				"SourceControl",
                "TargetPlatform",
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"MainFrame",
				"PropertyEditor",
			}
		);
	}
}
