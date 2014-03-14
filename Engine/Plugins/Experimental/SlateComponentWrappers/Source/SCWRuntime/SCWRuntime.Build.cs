// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SCWRuntime : ModuleRules
{
	public SCWRuntime(TargetInfo Target)
	{
		PrivateIncludePaths.Add("SlateComponentWrapperRuntime/Private");

		PublicDependencyModuleNames.AddRange(new string[] {
				"Core",
				"CoreUObject",
				"Engine",
                "InputCore"
			});

		PrivateDependencyModuleNames.AddRange(new string[] {
				"Slate",
                "EditorStyle"
			});
	}
}
