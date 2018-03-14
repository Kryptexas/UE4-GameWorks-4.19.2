// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class FacialAnimationEditor : ModuleRules
	{
		public FacialAnimationEditor(ReadOnlyTargetRules Target) : base(Target)
		{
			PublicIncludePaths.AddRange(
				new string[] {
					"Source/FacialAnimationEditor/Public",
					// ... add public include paths required here ...
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[] {
                    "Core",
                    "CoreUObject",
                    "InputCore",
                    "Engine",
                    "FacialAnimation",
                    "SlateCore",
                    "Slate",
					"EditorStyle",
					"UnrealEd",
                    "AudioEditor",
                    "WorkspaceMenuStructure",
                    "DesktopPlatform",
                    "PropertyEditor",
                    "TargetPlatform",
                    "Persona",
                }
			);
		}
	}
}
