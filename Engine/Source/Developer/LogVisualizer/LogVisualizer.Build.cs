// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LogVisualizer : ModuleRules
{
	public LogVisualizer(TargetInfo Target)
	{
		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"DesktopPlatform",
				"MainFrame",
			}
		);

		PublicIncludePaths.AddRange(
			new string[] {
				"Developer/TaskGraph/Public",
				"Runtime/Engine/Classes",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
                "InputCore",
				"Json",
				"Slate",
				"SlateCore",
                "EditorStyle",
				"Engine",
				"TaskGraph",
				"UnrealEd",
                "GameplayDebugger",
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"DesktopPlatform",
				"MainFrame",
			}
		);
	}
}
