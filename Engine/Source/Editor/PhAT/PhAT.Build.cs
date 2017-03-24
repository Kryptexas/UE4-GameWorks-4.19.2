// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PhAT : ModuleRules
{
	public PhAT(ReadOnlyTargetRules Target) : base(Target)
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
				"SlateCore",
                "EditorStyle",
				"LevelEditor",
				"UnrealEd",
                "Kismet",
                "Persona",
			}
        );

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"MainFrame",
				"PropertyEditor",
                "MeshUtilities",
			}
		);
	}
}
