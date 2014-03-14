// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PhAT : ModuleRules
{
	public PhAT(TargetInfo Target)
	{
		PublicIncludePaths.Add("Editor/UnrealEd/Public");

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
                "InputCore",
				"RenderCore",
				"Slate",
                "EditorStyle",
				"LevelEditor",
				"UnrealEd",
                "Kismet"
			}
			);

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"MainFrame",
				"PropertyEditor",
                "MeshUtilities"
			}
			);
	}
}
