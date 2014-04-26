// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class MeshUtilities : ModuleRules
{
	public MeshUtilities(TargetInfo Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string [] {
				"Core",
				"CoreUObject",
				"Engine",
				"RawMesh",
				"RenderCore", // For FPackedNormal
				"Slate",      // For FSlateTextureAtlas
				"SlateCore",
				"UnrealEd",
			}
		);

		AddThirdPartyPrivateStaticDependencies(Target, "nvTriStrip");
		AddThirdPartyPrivateStaticDependencies(Target, "ForsythTriOptimizer");
		AddThirdPartyPrivateStaticDependencies(Target, "MeshSimplifier");      

		if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
		{
			AddThirdPartyPrivateStaticDependencies(Target, "DX9");
		}

		if (UEBuildConfiguration.bCompileSimplygon == true)
		{
			AddThirdPartyPrivateDynamicDependencies(Target, "SimplygonMeshReduction");
		}
	}
}
