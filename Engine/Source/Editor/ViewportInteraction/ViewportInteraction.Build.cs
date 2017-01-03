// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

namespace UnrealBuildTool.Rules
{
    public class ViewportInteraction : ModuleRules
    {
        public ViewportInteraction(TargetInfo Target)
        {
            PublicDependencyModuleNames.AddRange(
                new string[] {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "InputCore",
                    "UnrealEd",
                    "Slate",
                    "SlateCore",
                }
            );

            PrivateIncludePathModuleNames.AddRange(
                new string[] {
					"LevelEditor"
                }
            );
		}
	}
}