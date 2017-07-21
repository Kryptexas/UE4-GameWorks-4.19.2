// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BlueprintProfiler : ModuleRules
{
    public BlueprintProfiler(ReadOnlyTargetRules Target) : base(Target)
    {
		PrivateIncludePaths.Add("Private");

        PrivateDependencyModuleNames.AddRange(
            new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"EngineSettings",
			}
            );

        if (Target.bBuildEditor == true)
        {
            PrivateDependencyModuleNames.Add("UnrealEd");
            PrivateDependencyModuleNames.Add("Kismet");
            PrivateDependencyModuleNames.Add("GraphEditor");
            PrivateDependencyModuleNames.Add("BlueprintGraph");
            PrivateDependencyModuleNames.Add("EditorStyle");
            PrivateDependencyModuleNames.Add("InputCore");
            PrivateDependencyModuleNames.Add("Slate");
            PrivateDependencyModuleNames.Add("SlateCore");
        }
    }
}

