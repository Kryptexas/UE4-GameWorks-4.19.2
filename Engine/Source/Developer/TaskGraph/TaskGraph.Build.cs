// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TaskGraph : ModuleRules
{
	public TaskGraph(TargetInfo Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"Slate",
                "EditorStyle",
				"Engine",
                "InputCore"
			}
			);
	}
}
