// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

namespace UnrealBuildTool.Rules
{
    public class ViewportInteraction : ModuleRules
    {
        public ViewportInteraction(ReadOnlyTargetRules Target) : base(Target)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[] {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "InputCore",
                    "UnrealEd",
                    "Slate",
                    "SlateCore",
                    "HeadMountedDisplay"
                }
            );

            PrivateIncludePathModuleNames.AddRange(
                new string[] {
					"LevelEditor"
                }
            );

            DynamicallyLoadedModuleNames.AddRange(
                new string[] {
                }
            );
        }
    }
}
