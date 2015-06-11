// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class RuntimeAssetCache : ModuleRules
{
	public RuntimeAssetCache(TargetInfo Target)
	{
		PrivateDependencyModuleNames.Add("Core");

		SharedPCHHeaderFile = "Runtime/RuntimeAssetCache/Public/RuntimeAssetCachePublicPCH.h";
	}
}
