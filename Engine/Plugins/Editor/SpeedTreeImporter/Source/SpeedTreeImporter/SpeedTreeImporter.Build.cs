// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class SpeedTreeImporter : ModuleRules
	{
		public SpeedTreeImporter(TargetInfo Target)
		{
			PublicIncludePaths.AddRange(
				new string[] {
				}
				);

			PrivateIncludePaths.AddRange(
				new string[] {
                    "MainFrame",
					"Editor/SpeedTreeImporter/Private",
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
                    "Core",
				    "CoreUObject",
				    "Engine",
				    "Slate",
					"SlateCore",
                    "EditorStyle",
                    "InputCore",
				    "RawMesh",
				    "SpeedTree",
                    "UnrealEd",
                    "MainFrame"
				}
				);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
				}
				);
		}
	}
}