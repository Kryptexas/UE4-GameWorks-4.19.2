// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class NewLogVisualizer : ModuleRules
{
	public NewLogVisualizer(TargetInfo Target)
	{
        bIsAutoStartupModule = true;
        
        PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"DesktopPlatform",
				"MainFrame",
                "SequencerWidgets",
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
                "SequencerWidgets",
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
