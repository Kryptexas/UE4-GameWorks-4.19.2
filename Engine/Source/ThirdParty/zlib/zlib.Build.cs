// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;

public class zlib : ModuleRules
{
	public zlib(TargetInfo Target)
	{
		Type = ModuleType.External;

		string zlibPath = UEBuildConfiguration.UEThirdPartyDirectory + "zlib/zlib-1.2.5/";

		PublicIncludePaths.Add(zlibPath + "inc");

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicLibraryPaths.Add(zlibPath + "Lib/Win64");
			PublicAdditionalLibraries.Add("zlib_64.lib");
		}
		else if (Target.Platform == UnrealTargetPlatform.Win32 || (Target.Platform == UnrealTargetPlatform.HTML5 && Target.Architecture == "-win32"))
		{
			PublicLibraryPaths.Add(zlibPath + "Lib/Win32");
			PublicAdditionalLibraries.Add("zlib.lib");
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac ||
				 Target.Platform == UnrealTargetPlatform.IOS)
		{
			PublicAdditionalLibraries.Add("z");
		}
		else if (Target.Platform == UnrealTargetPlatform.Android)
		{
			PublicAdditionalLibraries.Add("z");
		}
        else if (Target.Platform == UnrealTargetPlatform.HTML5)
        {
            PublicAdditionalLibraries.Add(zlibPath + "Lib/HTML5/zlib.bc");
        }
	}
}
