// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Niagara : ModuleRules
{
    public Niagara(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateDependencyModuleNames.AddRange(
            new string[] {
				"Core",
                "Engine",
				"RenderCore",
                "ShaderCore",
                "VectorVM",
                "RHI"
			}
        );


        PublicDependencyModuleNames.AddRange(
            new string[] {
                "MovieScene",
                "CoreUObject",
                "VectorVM",
                "RHI",
            }
        );


        PrivateIncludePaths.AddRange(
            new string[] {
                "Runtime/Niagara/Private",
                "Runtime/Engine/Private",
                "Runtime/Engine/Classes/Engine"
            })
        ;
    }
}
