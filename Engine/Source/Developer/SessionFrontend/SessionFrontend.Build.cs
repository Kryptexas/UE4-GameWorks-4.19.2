// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SessionFrontend : ModuleRules
{
	public SessionFrontend(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"Slate",
                "EditorStyle",
			}
		);

		PublicIncludePathModuleNames.AddRange(
			new string[] {
				"SessionServices",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"DesktopPlatform",
                "InputCore",
				"SlateCore",

				// @todo gmp: remove these dependencies by making the session front-end extensible
				"AutomationWindow",
				"ScreenShotComparison",
				"ScreenShotComparisonTools",
				"Profiler",
				"TargetPlatform",
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"Messaging",
				"TargetDeviceServices",
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
