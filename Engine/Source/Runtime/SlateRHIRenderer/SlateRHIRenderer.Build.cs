// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SlateRHIRenderer : ModuleRules
{
    public SlateRHIRenderer(TargetInfo Target)
	{
        PrivateIncludePaths.Add("Runtime/SlateRHIRenderer/Private");

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
                "InputCore",
				"Slate",
                "Engine",
                "RHI",
                "RenderCore",
                "ShaderCore",
                "ImageWrapper"
			}
		);


	}
}
