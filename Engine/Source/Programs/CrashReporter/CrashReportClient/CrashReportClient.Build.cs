// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CrashReportClient : ModuleRules
{
	public CrashReportClient(TargetInfo Target)
	{
		PublicIncludePaths.Add("Runtime/Launch/Public");

		PrivateDependencyModuleNames.AddRange(
			new string[] 
			{
				"Core",
				"CrashDebugHelper",
				"HTTP",
				"Projects",
				"XmlParser",
			}
			);

		if (Target.Platform != UnrealTargetPlatform.Linux)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[] 
				{
					"Slate",
					"StandaloneRenderer",
					"DesktopPlatform",
					"MessageLog",
				}
			);
		}

		PrivateIncludePaths.Add("Runtime/Launch/Private");		// For LaunchEngineLoop.cpp include
	}
}
