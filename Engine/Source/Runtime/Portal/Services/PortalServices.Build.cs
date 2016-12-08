// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class PortalServices : ModuleRules
	{
		public PortalServices(TargetInfo Target)
		{
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
				}
			);

            Definitions.Add("WITH_PORTAL_SERVICES=1");
		}
	}
}
