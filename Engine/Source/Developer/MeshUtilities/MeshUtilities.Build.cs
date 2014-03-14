// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class MeshUtilities : ModuleRules
{
	public MeshUtilities(TargetInfo Target)
	{
		PrivateDependencyModuleNames.Add("Core");
		PrivateDependencyModuleNames.Add("CoreUObject");
		PrivateDependencyModuleNames.Add("Engine");
		PrivateDependencyModuleNames.Add("RenderCore"); // For FPackedNormal.
        PrivateDependencyModuleNames.Add("Slate");      // For FSlateTextureAtlas
        PrivateDependencyModuleNames.Add("UnrealEd");
		AddThirdPartyPrivateStaticDependencies(Target, "nvTriStrip");
		AddThirdPartyPrivateStaticDependencies(Target, "ForsythTriOptimizer");
		AddThirdPartyPrivateStaticDependencies(Target, "MeshSimplifier");
		PrivateDependencyModuleNames.Add("RawMesh");
        

		if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
		{
			AddThirdPartyPrivateStaticDependencies(Target, "DX9");
		}

		if (UEBuildConfiguration.bCompileSimplygon == true)
		{
			AddThirdPartyPrivateDynamicDependencies(Target, "SimplygonMeshReduction");
			Definitions.Add("WITH_SIMPLYGON=1");
		}
	}
}