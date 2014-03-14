// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ScreenShotComparisonTools : ModuleRules
{
	public ScreenShotComparisonTools(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Messaging"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"AutomationMessages",
				"CoreUObject",
				"Networking",
				"UnrealEdMessages",
				"Slate",
                "EditorStyle",
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[]
			{
				"Messaging"
			}
		);

		PrivateIncludePaths.AddRange(
			new string[]
			{
				"Developer/ScreenShotComparisonTools/Private"
			}
		);
	}
}
