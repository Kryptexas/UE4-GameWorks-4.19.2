// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SessionLauncher : ModuleRules
{
	public SessionLauncher(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"TargetPlatform",
			}
		);

		PublicIncludePathModuleNames.AddRange(
			new string[] {
				"LauncherServices",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"DesktopPlatform",
                "InputCore",
				"Slate",
				"SlateCore",
                "EditorStyle",
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"SessionServices",
				"TargetDeviceServices",
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				"Developer/SessionLauncher/Private",
				"Developer/SessionLauncher/Private/Models",
				"Developer/SessionLauncher/Private/Widgets",
				"Developer/SessionLauncher/Private/Widgets/Build",
				"Developer/SessionLauncher/Private/Widgets/Cook",
				"Developer/SessionLauncher/Private/Widgets/Deploy",
				"Developer/SessionLauncher/Private/Widgets/Launch",
				"Developer/SessionLauncher/Private/Widgets/Package",
				"Developer/SessionLauncher/Private/Widgets/Project",
				"Developer/SessionLauncher/Private/Widgets/Shared",
				"Developer/SessionLauncher/Private/Widgets/Toolbar",
			}
		);
	}
}
