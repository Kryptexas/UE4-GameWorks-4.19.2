// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class AnimGraph : ModuleRules
{
	public AnimGraph(TargetInfo Target)
	{
		PrivateIncludePaths.Add("Editor/AnimGraph/Private");

		PublicDependencyModuleNames.AddRange(
			new string[] { 
				"Core", 
				"CoreUObject", 
				"Engine", 
				"Slate",
				"BlueprintGraph"
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"UnrealEd",
                "GraphEditor",
			}
			);

		CircularlyReferencedDependentModules.AddRange(
			new string[] {
                "UnrealEd",
                "GraphEditor",
            }
			); 
	}
}
