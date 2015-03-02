// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CrashDebugHelper : ModuleRules
{
	public CrashDebugHelper(TargetInfo Target)
	{
		PrivateIncludePaths.Add("Developer/CrashDebugHelper/Private");
		PrivateIncludePaths.Add("ThirdParty/PLCrashReporter/plcrashreporter-master-5ae3b0a/Source");

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"SourceControl",
				"DatabaseSupport"
			}
		);
	}
}
