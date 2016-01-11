// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GeometryCache : ModuleRules
{
	public GeometryCache(TargetInfo Target)
	{
        PublicIncludePaths.Add("Runtime/GeometryCache/Public");
        PrivateIncludePaths.Add("Runtime/GeometryCache/Private");
        
        PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject", // @todo Mac: for some reason it's needed to link in debug on Mac
				"Engine",
				"Slate",
				"SlateCore",
                "InputCore",
                "EditorStyle",
                "UnrealEd",
                "AssetTools",
                "RenderCore",
                "ShaderCore",
                "RHI"
			}
		);
	}
}
