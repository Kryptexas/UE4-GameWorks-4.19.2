// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ScreenShotComparison : ModuleRules
{
	public ScreenShotComparison(TargetInfo Target)
	{
		PrivateIncludePaths.AddRange(
			new string[]
			{
				"Developer/ScreenShotComparison/Private",
				"Developer/ScreenShotComparison/Private/Widgets",
				"Developer/ScreenShotComparison/Private/Models",
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "InputCore",
				"SessionServices",
				"Slate",
                "EditorStyle",
				"ScreenShotComparisonTools",
			}
			);
	}
}
