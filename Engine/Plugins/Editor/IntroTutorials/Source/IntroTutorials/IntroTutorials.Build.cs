// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class IntroTutorials : ModuleRules
	{
		public IntroTutorials(TargetInfo Target)
		{
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject", // @todo Mac: for some reason CoreUObject and Engine are needed to link in debug on Mac
					"Engine", // @todo Mac: for some reason CoreUObject and Engine are needed to link in debug on Mac
                    "InputCore",
                    "Slate",
                    "EditorStyle",
                    "Documentation",
					"GraphEditor",
					"BlueprintGraph",
					"MessageLog"
				}
			);

			PrivateIncludePaths.AddRange(
				new string[] {
					"Editor/IntroTutorials/Private",
				}
			);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{				
                    "UnrealEd",
                    "Kismet",
                    "PlacementMode"
				}
			);

			PrivateIncludePathModuleNames.AddRange(
				new string[] {
					"MainFrame",
				}
			);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
                    "Documentation",
					"MainFrame",
				}
			);
		}
	}
}