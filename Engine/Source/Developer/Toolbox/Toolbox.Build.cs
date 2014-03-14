// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Toolbox : ModuleRules
{
	public Toolbox(TargetInfo Target)
	{
		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"GammaUI",
				"MainFrame",
				"ModuleUI"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
                "InputCore",
				"Slate",
                "EditorStyle"
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"GammaUI", 
				"MainFrame",
				"ModuleUI"
			}
		);
	}
}