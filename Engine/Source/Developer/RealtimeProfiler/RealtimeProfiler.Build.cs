// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RealtimeProfiler : ModuleRules
{
	public RealtimeProfiler(TargetInfo Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject", // @todo Mac: for some reason it's needed to link in debug on Mac
                "InputCore",
				"Slate",
                "EditorStyle",
				"TaskGraph",
				"Engine"
			}
			);
	}
}
