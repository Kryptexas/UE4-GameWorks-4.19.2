// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PackagesDialog : ModuleRules
{
	public PackagesDialog(TargetInfo Target)
	{
		PrivateIncludePathModuleNames.Add("AssetTools");

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core", 
				"CoreUObject", 
				"Engine", 
                "InputCore",
				"Slate", 
                "EditorStyle",
				"UnrealEd",
				"SourceControl"
			}
			);

		DynamicallyLoadedModuleNames.Add("AssetTools");
	}
}
