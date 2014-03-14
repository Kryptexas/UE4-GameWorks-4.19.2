// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

using System;
using System.IO;

public class HTML5TargetPlatform : ModuleRules
{
	public HTML5TargetPlatform(TargetInfo Target)
	{
		BinariesSubFolder = "HTML5";

		PrivateIncludePathModuleNames.Add("TargetPlatform");

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Settings",
				"Sockets"
			}
			);

		if (UEBuildConfiguration.bCompileAgainstEngine)
		{
			PrivateDependencyModuleNames.Add("Engine");
//			PrivateIncludePathModuleNames.Add("TextureCompressor");
		}

		PrivateIncludePaths.AddRange(
			new string[]
			{
				"Developer/HTML5TargetPlatform/Classes"
			}
		);
	}
}
