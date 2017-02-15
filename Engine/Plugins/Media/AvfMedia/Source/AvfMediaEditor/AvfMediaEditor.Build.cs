// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class AvfMediaEditor : ModuleRules
	{
		public AvfMediaEditor(TargetInfo Target)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"Core",
					"CoreUObject",
					"MediaAssets",
					"UnrealEd",
                }
			);

			PrivateIncludePaths.AddRange(
				new string[] {
					"AvfMediaEditor/Private",
				}
			);
		}
	}
}
