// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Persona : ModuleRules
{
	public Persona(TargetInfo Target)
	{
		PrivateIncludePaths.Add("Editor/Persona/Private");	// For PCH includes (because they don't work with relative paths, yet)

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"AssetRegistry", 
				"MainFrame",
				"DesktopPlatform"
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core", 
				"CoreUObject", 
				"Slate", 
                "EditorStyle",
				"Engine", 
				"UnrealEd", 
				"GraphEditor", 
                "InputCore",
				"Kismet", 
				"KismetWidgets",
				"AnimGraph",
                "PropertyEditor",
				"EditorWidgets",
                "BlueprintGraph",
			}
			);

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"ContentBrowser",
				"Documentation",
				"MainFrame",
				"DesktopPlatform",
                "PropertyEditor"
			}
			);
	}
}
