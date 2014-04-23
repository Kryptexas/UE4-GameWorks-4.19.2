// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UMG : ModuleRules
{
	public UMG(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"ShaderCore",
				"RenderCore",
                "InputCore",
				"RHI",
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Slate"
			}
			);
	}
}
