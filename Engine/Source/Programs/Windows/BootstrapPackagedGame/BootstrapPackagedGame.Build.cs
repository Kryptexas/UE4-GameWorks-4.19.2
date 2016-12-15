// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BootstrapPackagedGame : ModuleRules
{
	public BootstrapPackagedGame(TargetInfo Target)
	{
		PublicAdditionalLibraries.Add("shlwapi.lib");
		bEnableUndefinedIdentifierWarnings = false;
	}
}
