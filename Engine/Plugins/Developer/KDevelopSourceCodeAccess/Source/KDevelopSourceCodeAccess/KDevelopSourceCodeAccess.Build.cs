// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class KDevelopSourceCodeAccess : ModuleRules
	{
		public KDevelopSourceCodeAccess(ReadOnlyTargetRules Target) : base(Target)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"SourceCodeAccess",
					"DesktopPlatform",
				}
			);

			if (UEBuildConfiguration.bBuildEditor)
			{
				PrivateDependencyModuleNames.Add("HotReload");
			}
		}
	}
}
