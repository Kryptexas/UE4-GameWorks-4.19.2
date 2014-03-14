// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class elftoolchain : ModuleRules
{
	public elftoolchain(TargetInfo Target)
	{
		Type = ModuleType.External;

		PublicIncludePaths.Add(UEBuildConfiguration.UEThirdPartyDirectory + "elftoolchain/include");

        if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            PublicLibraryPaths.Add(UEBuildConfiguration.UEThirdPartyDirectory + "elftoolchain/lib/Linux");
            PublicAdditionalLibraries.Add("elf");
            PublicAdditionalLibraries.Add("dwarf");
        }
	}
}
