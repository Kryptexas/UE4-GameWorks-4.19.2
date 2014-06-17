// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MetalRHI : ModuleRules
{	
	public MetalRHI(TargetInfo Target)
	{

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"Engine",
				"RHI",
				"RenderCore",
				"ShaderCore"
			}
			);
			
		PublicFrameworks.Add("Metal");
	}
}
