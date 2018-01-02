// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PinnedCommandList : ModuleRules
{
    public PinnedCommandList(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.Add("Editor/PinnedCommandList/Private");

        PublicIncludePathModuleNames.AddRange(
            new string[] {
            }
        );

        PublicDependencyModuleNames.AddRange(
            new string[] {
            }
        );

        PrivateIncludePathModuleNames.AddRange(
            new string[] {
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "Core",
                "CoreUObject",
				"ApplicationCore",
                "Slate",
                "SlateCore",
                "EditorStyle",
                "InputCore",
            }
        );

        DynamicallyLoadedModuleNames.AddRange(
            new string[] {
            }
        );
    }
}
