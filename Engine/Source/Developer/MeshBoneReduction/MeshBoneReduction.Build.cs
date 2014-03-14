// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class MeshBoneReduction : ModuleRules
{
    public MeshBoneReduction(TargetInfo Target)
	{
		PublicIncludePaths.Add("Developer/MeshBoneReduction/Public");
		PrivateDependencyModuleNames.Add("Core");
		PrivateDependencyModuleNames.Add("CoreUObject");
		PrivateDependencyModuleNames.Add("Engine");
        PrivateDependencyModuleNames.Add("RenderCore");
	}
}
