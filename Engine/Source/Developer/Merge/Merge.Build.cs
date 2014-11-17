// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class Merge : ModuleRules
	{
        public Merge(TargetInfo Target)
		{
			PublicIncludePaths.AddRange(
				new string[] {
				}
				);

			PrivateIncludePaths.AddRange(
				new string[] {
					"Developer/Merge/Private",
				    "Kismet",
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
				    "AssetTools",
				    "CoreUObject",
				    "EditorStyle",
				    "Engine", // needed so that we can clone blueprints...
				    "GraphEditor",
				    "InputCore",
				    "Kismet",
				    "Slate",
				    "SlateCore",
				    "SourceControl",
				    "UnrealEd",
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