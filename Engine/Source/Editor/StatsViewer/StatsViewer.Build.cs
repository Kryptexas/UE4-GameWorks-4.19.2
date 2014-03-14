// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class StatsViewer : ModuleRules
{
	public StatsViewer(TargetInfo Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"Slate",
                "EditorStyle",
				"RHI",
				"UnrealEd"
			}
		);

        PrivateIncludePathModuleNames.AddRange(
			new string[] {
                "PropertyEditor"
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"PropertyEditor"
			}
		);

        PrivateIncludePaths.AddRange(
            new string[] {
                "Editor/StatsViewer/Private",
				"Editor/StatsViewer/Private/StatsPages",
                "Editor/StatsViewer/Private/StatsEntries"
			}
		);
	}
}