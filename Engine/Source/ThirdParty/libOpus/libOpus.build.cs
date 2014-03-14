// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class libOpus : ModuleRules
{
	public libOpus(TargetInfo Target)
	{
		/** Mark the current version of the library */
		string OpusVersion = "1.1";
		Type = ModuleType.External;

		PublicIncludePaths.Add(UEBuildConfiguration.UEThirdPartyDirectory + "libOpus/opus-" + OpusVersion + "/include");
		string LibraryPath = UEBuildConfiguration.UEThirdPartyDirectory + "libOpus/opus-" + OpusVersion + "/";

		if ((Target.Platform == UnrealTargetPlatform.Win64) ||
			(Target.Platform == UnrealTargetPlatform.Win32))
		{
			LibraryPath += "win32/VS2012/";
			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				LibraryPath += "x64/";
			}
			else
			{
				LibraryPath += "win32/";
			}

			LibraryPath += "Release/";

			PublicLibraryPaths.Add(LibraryPath);

 			PublicAdditionalLibraries.Add("silk_common.lib");
 			PublicAdditionalLibraries.Add("silk_float.lib");
 			PublicAdditionalLibraries.Add("celt.lib");
			PublicAdditionalLibraries.Add("opus.lib");
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			LibraryPath += "/Mac/libopus.a";
			PublicAdditionalLibraries.Add(LibraryPath);
		}
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			LibraryPath += "Linux/CentOS6/";
            PublicLibraryPaths.Add(LibraryPath);
            PublicAdditionalLibraries.Add("opus");
		}
	}
}