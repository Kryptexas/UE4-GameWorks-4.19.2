// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DsymExporter : ModuleRules
{
	public DsymExporter( TargetInfo Target )
	{
		PrivateIncludePathModuleNames.Add( "Launch" );
		PrivateIncludePaths.Add( "Runtime/Launch/Private" );
	
		PrivateDependencyModuleNames.AddRange(
			new string[] 
			{ 
				"Core",
				"Projects"
			}
			);
	}
}
