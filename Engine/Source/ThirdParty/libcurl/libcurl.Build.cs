// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class libcurl : ModuleRules
{
	public libcurl(TargetInfo Target)
	{
		Type = ModuleType.External;

		PublicIncludePaths.Add(UEBuildConfiguration.UEThirdPartyDirectory + "libcurl/include");

        if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            PublicLibraryPaths.Add(UEBuildConfiguration.UEThirdPartyDirectory + "libcurl/lib/Linux");
            PublicAdditionalLibraries.Add("curl");
            PublicAdditionalLibraries.Add("crypto");
            PublicAdditionalLibraries.Add("ssl");
            PublicAdditionalLibraries.Add("dl");
        }
	}
}
