// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class BlueprintStats : ModuleRules
	{
        public BlueprintStats(TargetInfo Target)
		{
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"UnrealEd",
					"GraphEditor",
					"BlueprintGraph",
					"MessageLog"
				}
			);
		}
	}
}