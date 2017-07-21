// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;

public class HACD : ModuleRules
{
    public HACD(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		string HACDDirectory = Target.UEThirdPartySourceDirectory + "HACD/HACD_1.0/";
		string HACDLibPath = HACDDirectory;
		PublicIncludePaths.Add(HACDDirectory + "public");

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			HACDLibPath = HACDLibPath + "lib/Win64/VS" + Target.WindowsPlatform.GetVisualStudioCompilerVersionName() + "/";
			PublicLibraryPaths.Add(HACDLibPath);

            if (Target.Configuration == UnrealTargetConfiguration.Debug && Target.bDebugBuildsActuallyUseDebugCRT)
            {
                PublicAdditionalLibraries.Add("HACDd_64.lib");
            }
            else
            {
                PublicAdditionalLibraries.Add("HACD_64.lib");
            }
		}
		else if (Target.Platform == UnrealTargetPlatform.Win32)
		{
			HACDLibPath = HACDLibPath + "lib/Win32/VS" + Target.WindowsPlatform.GetVisualStudioCompilerVersionName() + "/";
			PublicLibraryPaths.Add(HACDLibPath);

			if (Target.Configuration == UnrealTargetConfiguration.Debug && Target.bDebugBuildsActuallyUseDebugCRT)
			{
				PublicAdditionalLibraries.Add("HACDd.lib");
			}
			else
			{
				PublicAdditionalLibraries.Add("HACD.lib");
			}
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			string LibPath = HACDDirectory + "lib/Mac/";
			if (Target.Configuration == UnrealTargetConfiguration.Debug && Target.bDebugBuildsActuallyUseDebugCRT)
			{
				PublicAdditionalLibraries.Add(LibPath + "libhacd_debug.a");
			}
			else
			{
				PublicAdditionalLibraries.Add(LibPath + "libhacd.a");
			}
		}
	}
}

