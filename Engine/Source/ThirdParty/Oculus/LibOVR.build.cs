// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LibOVR : ModuleRules
{
	public LibOVR(TargetInfo Target)
	{
		/** Mark the current version of the Oculus SDK */
		string LibOVRVersion = "025";
		Type = ModuleType.External;

		if ((Target.Platform == UnrealTargetPlatform.Win64) ||
			(Target.Platform == UnrealTargetPlatform.Win32))
		{
            PublicIncludePaths.Add(UEBuildConfiguration.UEThirdPartyDirectory + "Oculus/LibOVR_" + LibOVRVersion + "/Include");

            string LibraryPath = UEBuildConfiguration.UEThirdPartyDirectory + "Oculus/LibOVR_" + LibOVRVersion + "/Lib/";
			string LibraryName = "libovr";
			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				LibraryPath += "x64/";
				LibraryName += "64";
			}
            else if (Target.Platform == UnrealTargetPlatform.Win32)
            {
                LibraryPath += "Win32/";
            }
			PublicLibraryPaths.Add(LibraryPath);
			PublicAdditionalLibraries.Add(LibraryName + ".lib");
            //PublicAdditionalLibraries.Add(LibraryName + "d.lib");
			//PublicDelayLoadDLLs.Add(LibraryName + ".dll");
		}
	}
}
