// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ScreenShotComparisonTools : ModuleRules
{
	public ScreenShotComparisonTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"AutomationMessages",
				"UnrealEdMessages",
				"Slate",
                "EditorStyle",
				"ImageWrapper",
				"Json",
				"JsonUtilities"
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"Messaging"
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				"Developer/ScreenShotComparisonTools/Private"
			}
		);
	}
}
