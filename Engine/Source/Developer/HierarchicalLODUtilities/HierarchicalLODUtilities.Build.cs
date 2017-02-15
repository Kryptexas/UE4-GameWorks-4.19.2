// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;

public class HierarchicalLODUtilities : ModuleRules
{
    public HierarchicalLODUtilities(TargetInfo Target)
	{
        PublicIncludePaths.Add("Developer/HierarchicalLODUtilities/Public");

        PublicDependencyModuleNames.AddRange(
            new string[]
			{
				"Core",
				"CoreUObject",
			}
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
			{
				"Engine",
				"UnrealEd",
                "Projects"
			}
        );

        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                "MeshUtilities"
            }
        );
	}
}