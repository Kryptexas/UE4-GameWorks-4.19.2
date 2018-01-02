// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class NiagaraVertexFactories : ModuleRules
{
    public NiagaraVertexFactories(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateDependencyModuleNames.AddRange(
            new string[] {
				"Core",
				"Engine",		
				"RenderCore",
				"ShaderCore",
                "RHI",
			}
        );

		PublicIncludePathModuleNames.AddRange(
			new string[] {
				"Niagara",
			}
		);

        PublicDependencyModuleNames.AddRange(
            new string[] {
				"VectorVM",
            }
        );


        PrivateIncludePaths.AddRange(
            new string[] {
            })
        ;
    }
}
