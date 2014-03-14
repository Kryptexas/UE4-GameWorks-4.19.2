// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class EditorWidgets : ModuleRules
{
	public EditorWidgets(TargetInfo Target)
	{
		PrivateIncludePathModuleNames.Add("AssetRegistry");

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
                "InputCore",
				"Slate",
                "EditorStyle",
				"UnrealEd"
			}
			);

		DynamicallyLoadedModuleNames.Add("AssetRegistry");
	}
}
