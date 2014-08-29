// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CrashReportClient : ModuleRules
{
	public CrashReportClient(TargetInfo Target)
	{
		PublicIncludePaths.AddRange
		(
			new string[] 
			{ 
				"Runtime/Launch/Public",
				"Programs/CrashReporter/CrashReportClient/Private",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] 
			{
				"Core",
				"CrashDebugHelper",
				"HTTP",
				"Projects",
				"XmlParser",
				"Analytics",
				"AnalyticsET",
			}
			);

		if (Target.Platform != UnrealTargetPlatform.Linux)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[] 
				{
					"Slate",
					"SlateCore",
					"SlateReflector",
					"StandaloneRenderer",
					"DesktopPlatform",
					"MessageLog",
				}
			);
		}

		PrivateIncludePaths.Add("Runtime/Launch/Private");		// For LaunchEngineLoop.cpp include
	}
}
