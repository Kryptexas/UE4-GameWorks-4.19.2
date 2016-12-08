// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DirectoryWatcher : ModuleRules
{
	public DirectoryWatcher(TargetInfo Target)
	{
		PrivateIncludePaths.Add("Developer/DirectoryWatcher/Private");

		PrivateDependencyModuleNames.Add("Core");
	}
}
