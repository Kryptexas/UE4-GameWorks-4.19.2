// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LinuxNoEditorTargetPlatform : ModuleRules
{
    public LinuxNoEditorTargetPlatform(TargetInfo Target)
    {
        BinariesSubFolder = "Linux";

        PrivateDependencyModuleNames.AddRange(
            new string[] {
				"Core",
				"TargetPlatform"
			}
        );

        if (UEBuildConfiguration.bCompileAgainstEngine)
        {
            PrivateDependencyModuleNames.AddRange(new string[] {
				"Engine"
				}
            );

            PrivateIncludePathModuleNames.Add("TextureCompressor");
        }

        PrivateIncludePaths.AddRange(
            new string[] {
				"Developer/Linux/LinuxTargetPlatform/Private"
			}
        );
    }
}
