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
				"Sockets",
			}
		);

		PrivateIncludePathModuleNames.Add("TargetPlatform");

		if (UEBuildConfiguration.bCompileAgainstEngine)
		{
			PrivateDependencyModuleNames.Add("Engine");
			PrivateIncludePathModuleNames.Add("TextureCompressor");		//@todo linux: LinuxTargetPlatform.Build
		}
	}
}
