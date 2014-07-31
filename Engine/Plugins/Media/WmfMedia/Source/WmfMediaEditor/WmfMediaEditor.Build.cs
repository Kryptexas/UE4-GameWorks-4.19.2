// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class WmfMediaEditor : ModuleRules
	{
		public WmfMediaEditor(TargetInfo Target)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[] {
                    "Core",
                    "CoreUObject",
					"UnrealEd",
				}
			);

			PrivateIncludePaths.AddRange(
				new string[] {
					"WmfMediaEditor/Private",
				}
			);
		}
	}
}
