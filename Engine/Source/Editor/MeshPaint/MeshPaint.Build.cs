// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MeshPaint : ModuleRules
{
    public MeshPaint(TargetInfo Target)
    {
		PrivateIncludePathModuleNames.Add("AssetTools");

        PrivateDependencyModuleNames.AddRange( 
            new string[] { 
                "Core", 
                "CoreUObject",
                "DesktopPlatform",
                "Engine", 
                "InputCore",
                "RenderCore",
                "RHI",
                "ShaderCore",
                "Slate",
                "EditorStyle",
                "UnrealEd",
                "RawMesh",
                "SourceControl"
            } 
        );

		DynamicallyLoadedModuleNames.Add("AssetTools");
    }
}
