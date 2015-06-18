// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProceduralMeshComponent : ModuleRules
{
    public ProceduralMeshComponent(TargetInfo Target)
    {
        PublicIncludePaths.Add("Runtime/ProceduralMeshComponent/Public");
        PrivateIncludePaths.Add("Runtime/ProceduralMeshComponent/Private");

        PublicDependencyModuleNames.AddRange(
            new string[] {
				"Core",
				"CoreUObject",
                "Engine",
                "RenderCore",
                "ShaderCore",
                "RHI"
			}
          );
    }
}

