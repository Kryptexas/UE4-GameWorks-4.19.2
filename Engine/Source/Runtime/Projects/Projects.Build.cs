// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class Projects : ModuleRules
	{
		public Projects(TargetInfo Target)
		{
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
				}
			);

			PrivateIncludePaths.AddRange(
				new string[]
				{
					"Runtime/Projects/Private",
				}
			);
		}
	}
}
