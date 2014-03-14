// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class BlueprintGraph : ModuleRules
{
	public BlueprintGraph(TargetInfo Target)
	{
		PrivateIncludePaths.AddRange(
            new string[] {
                "Editor/BlueprintGraph/Private",
                "Editor/KismetCompiler/Public",
            }
            );

		PublicDependencyModuleNames.AddRange(
			new string[] { 
				"Core", 
				"CoreUObject", 
				"Engine",
                "InputCore",
				"Slate",
                "EditorStyle",
			}
			);

		PrivateDependencyModuleNames.AddRange( 
			new string[] {
				"EditorStyle",
                "KismetCompiler",
				"UnrealEd",
                "GraphEditor",
			}
			);

		CircularlyReferencedDependentModules.AddRange(
            new string[] {
                "KismetCompiler",
                "UnrealEd",
                "GraphEditor",
            }
            ); 

	}
}
