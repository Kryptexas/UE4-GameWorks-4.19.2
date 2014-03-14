// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SourceControlWindows : ModuleRules
{
	public SourceControlWindows(TargetInfo Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core", 
				"CoreUObject", 
                "InputCore",
				"Engine", 
				"Slate", 
                "EditorStyle",
				"SourceControl", 
				"AssetTools",
                "UnrealEd"			// We need this dependency here because we use PackageTools.
			}
			);
	}
}
