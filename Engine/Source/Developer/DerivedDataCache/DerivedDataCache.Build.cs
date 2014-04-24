// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class DerivedDataCache : ModuleRules
{
	public DerivedDataCache(TargetInfo Target)
	{
		PrivateDependencyModuleNames.Add("Core");
		// Internal (NoRedist) module
		if (Directory.Exists(Path.Combine("Developer", "NotForLicensees", "DDCUtils")) && !UnrealBuildTool.UnrealBuildTool.BuildingRocket())
		{
			DynamicallyLoadedModuleNames.Add("DDCUtils");
		}
	}
}
