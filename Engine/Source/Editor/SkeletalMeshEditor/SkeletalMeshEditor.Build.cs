// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SkeletalMeshEditor : ModuleRules
{
	public SkeletalMeshEditor(ReadOnlyTargetRules Target) : base(Target)
	{
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
                "Persona",
                "SkeletonEditor",
                "Kismet",
                "KismetWidgets",
                "ActorPickerMode",
                "SceneDepthPickerMode",
                "MainFrame",
                "DesktopPlatform",
                "PropertyEditor",
                "RHI",
            }
		);

        PrivateIncludePathModuleNames.AddRange(
            new string[] {
                "PropertyEditor",
            }
        );

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
            }
		);
	}
}
