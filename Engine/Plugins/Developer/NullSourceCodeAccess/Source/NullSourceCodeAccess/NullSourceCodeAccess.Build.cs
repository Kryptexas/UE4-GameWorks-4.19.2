// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class NullSourceCodeAccess : ModuleRules
	{
		public NullSourceCodeAccess(TargetInfo Target)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"DesktopPlatform",
					"SourceCodeAccess"
				}
			);
		}
	}
}
