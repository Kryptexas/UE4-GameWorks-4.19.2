// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Paper2D : ModuleRules
{
	public Paper2D(TargetInfo Target)
	{
		PrivateIncludePaths.Add("Paper2D/Private");

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"ShaderCore",
				"RenderCore",
				"RHI",
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Slate"
			}
			);

		if (UEBuildConfiguration.bBuildEditor == true)
		{
			//@TODO: Needed for the triangulation code used for sprites (but only in editor mode)
			//@TOOD: Try to move the code dependent on the triangulation code to the editor-only module
			PrivateDependencyModuleNames.Add("UnrealEd");
		}

		// @TODO: Box2D integration is turned off for now, until we have a good way to distribute third party libraries with plugins (and Paper2D actually uses it!)
		Definitions.Add("WITH_BOX2D=0");
	}
}
