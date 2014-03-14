// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class UdpMessaging : ModuleRules
	{
		public UdpMessaging(TargetInfo Target)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Messaging",
					"Networking",
					"Settings",
					"Sockets",
				}
			);

			PrivateIncludePaths.AddRange(
				new string[] {
					"UdpMessaging/Private",
					"UdpMessaging/Private/Transport",
					"UdpMessaging/Private/Transport/Tests",
					"UdpMessaging/Private/Tunnel",
				}
			);
		}
	}
}
