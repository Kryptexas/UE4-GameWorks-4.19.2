// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class LauncherAutomatedService : ModuleRules
	{
		public LauncherAutomatedService(TargetInfo Target)
		{
			PrivateIncludePaths.Add("Runtime/LauncherAutomatedService/Private");

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"AutomationController",
					"Core",
					"CoreUObject",
					"LauncherServices",
					"Messaging",
					"TargetDeviceServices",
				}
				);
		}
	}
}
