// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class LauncherServices : ModuleRules
	{
		public LauncherServices(TargetInfo Target)
		{
			PublicDependencyModuleNames.AddRange(
			new string[]
				{
					"Core",
					"TargetDeviceServices",
				}
			);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"CoreUObject",
					"Messaging",
					"SessionMessages",
					"TargetPlatform",
					"UnrealEdMessages",
				}
			);

			PrivateIncludePaths.AddRange(
			new string[]
				{
					"Developer/LauncherServices/Private",
					"Developer/LauncherServices/Private/Devices",
					"Developer/LauncherServices/Private/Games",
					"Developer/LauncherServices/Private/Launcher",
					"Developer/LauncherServices/Private/Profiles",
				}
			);
		}
	}
}
