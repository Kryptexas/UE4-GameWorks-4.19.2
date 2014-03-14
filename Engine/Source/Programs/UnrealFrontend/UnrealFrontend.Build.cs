// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UnrealFrontend : ModuleRules
{
	public UnrealFrontend( TargetInfo Target )
	{
		PublicIncludePaths.Add("Runtime/Launch/Public");

		PrivateIncludePaths.AddRange(
			new string[] {
				"Programs/UnrealFrontend/Private",
				"Runtime/Launch/Private",					// for LaunchEngineLoop.cpp include
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"AutomationController",
				"Core",
				"DeviceManager",
				"Messaging",
				"ProfilerClient",
				"Projects",
				"SessionFrontend",
				"SessionLauncher",
				"Slate",
				"StandaloneRenderer",
				"TargetPlatform",
			}
		);

		// @todo: allow for better plug-in support in standalone Slate apps
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Networking",
				"Sockets",
				"UdpMessaging",
			}
		);
	}
}