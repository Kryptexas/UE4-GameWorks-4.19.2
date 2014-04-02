// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LinuxTargetPlatform : ModuleRules
{
    public LinuxTargetPlatform(TargetInfo Target)
	{
        BinariesSubFolder = "Linux";

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
				"Core",
				"CoreUObject",
				"Settings",
				"TargetPlatform"
			}
        );

        PublicIncludePaths.AddRange(
            new string[]
			{
				"Runtime/Core/Public/Linux"
			}
        );

        if (UEBuildConfiguration.bCompileAgainstEngine)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[] 
                {
				    "Engine"
				}
            );

            PrivateIncludePathModuleNames.Add("TextureCompressor");
        }

        PrivateIncludePaths.AddRange(
            new string[]
			{
				"Developer/LinuxTargetPlatform/Classes"
			}
        );
	}
}
