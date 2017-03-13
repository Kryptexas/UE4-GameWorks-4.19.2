// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class Projects : ModuleRules
	{
		public Projects(ReadOnlyTargetRules Target) : base(Target)
		{
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"Json",
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
