// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SessionFrontend : ModuleRules
{
	public SessionFrontend(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"SessionServices",
				"Slate",
                "EditorStyle",
			}
		); 
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"DesktopPlatform",
                "InputCore",
				"TargetDeviceServices",

				// @todo gmp: remove these dependencies by making the session front-end extensible
				"AutomationWindow",
				"ScreenShotComparison",
				"ScreenShotComparisonTools",
				"Profiler"
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				"Developer/SessionFrontend/Private",
				"Developer/SessionFrontend/Private/Models",
				"Developer/SessionFrontend/Private/Widgets",
				"Developer/SessionFrontend/Private/Widgets/Browser",
				"Developer/SessionFrontend/Private/Widgets/Console",
			}
		);
	}
}
