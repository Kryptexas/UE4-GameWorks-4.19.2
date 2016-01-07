// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class CodeLiteSourceCodeAccess : ModuleRules
	{
		public CodeLiteSourceCodeAccess(TargetInfo Target)
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
