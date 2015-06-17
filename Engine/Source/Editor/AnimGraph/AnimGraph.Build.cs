// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

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
				"AnimGraphRuntime",
				"BlueprintGraph",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"SlateCore",
				"UnrealEd",
                "GraphEditor",
				"PropertyEditor"
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
