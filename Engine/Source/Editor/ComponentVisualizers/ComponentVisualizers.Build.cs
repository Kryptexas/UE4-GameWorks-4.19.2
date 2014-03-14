// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ComponentVisualizers : ModuleRules
{
	public ComponentVisualizers(TargetInfo Target)
	{
		PrivateIncludePaths.Add("Editor/ComponentVisualizers/Private");	// For PCH includes (because they don't work with relative paths, yet)

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"UnrealEd",
                "PropertyEditor",
			}
			);
	}
}
