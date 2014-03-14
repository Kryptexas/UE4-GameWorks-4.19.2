// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UserFeedback : ModuleRules
{
    public UserFeedback(TargetInfo Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"UnrealEd",
			}
		);
		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"Slate",
                "EditorStyle",
				"Engine",
				"Analytics",
                "InputCore",
			}
		);
	}
}
