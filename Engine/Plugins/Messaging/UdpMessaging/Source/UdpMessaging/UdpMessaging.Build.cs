// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class UdpMessaging : ModuleRules
	{
		public UdpMessaging(TargetInfo Target)
		{
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Networking",
				}
			); 

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"Core",
					"CoreUObject",
					"Sockets",
				}
			);

			PrivateIncludePathModuleNames.AddRange(
				new string[] {
					"Messaging",
					"Settings",
				}
			);

			PrivateIncludePaths.AddRange(
				new string[] {
					"UdpMessaging/Private",
					"UdpMessaging/Private/Shared",
					"UdpMessaging/Private/Transport",
					"UdpMessaging/Private/Transport/Tests",
					"UdpMessaging/Private/Tunnel",
				}
			);
		}
	}
}
