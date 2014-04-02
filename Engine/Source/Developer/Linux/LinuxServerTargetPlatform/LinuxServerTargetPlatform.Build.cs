// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LinuxServerTargetPlatform : ModuleRules
{
    public LinuxServerTargetPlatform(TargetInfo Target)
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
