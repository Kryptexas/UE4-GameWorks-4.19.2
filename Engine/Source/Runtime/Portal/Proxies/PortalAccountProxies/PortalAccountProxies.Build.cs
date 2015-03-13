// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class PortalAccountProxies : ModuleRules
	{
		public PortalAccountProxies(TargetInfo Target)
		{
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
				}
			); 

			PrivateIncludePaths.AddRange(
				new string[] {
					"Runtime/Portal/Proxies/PortalAccountProxies/Private",
				}
			);
		}
	}
}
