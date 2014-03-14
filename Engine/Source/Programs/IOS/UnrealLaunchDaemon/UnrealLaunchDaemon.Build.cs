// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UnrealLaunchDaemon : ModuleRules
{
	public UnrealLaunchDaemon( TargetInfo Target )
	{
		PublicIncludePaths.AddRange(
			new string[] {
				"Runtime/Launch/Public"
			}
		);

		PrivateIncludePaths.Add("Runtime/Launch/Private");

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"Messaging",
				"Networking",
				"NetworkFile",
				"Projects",
				"StreamingFile",
				"Slate",
				"EditorStyle",
				"StandaloneRenderer",
				"LaunchDaemonMessages",
				"Projects",
				"UdpMessaging"
			}
		);
	}
}