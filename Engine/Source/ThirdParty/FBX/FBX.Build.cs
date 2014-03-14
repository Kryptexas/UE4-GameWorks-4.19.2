// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FBX : ModuleRules
{
	public FBX(TargetInfo Target)
	{
		Type = ModuleType.External;

		Definitions.Add("FBXSDK_NEW_API");

		PublicIncludePaths.AddRange(
			new string[] {
					UEBuildConfiguration.UEThirdPartyDirectory + "FBX/2013.3/include",
					UEBuildConfiguration.UEThirdPartyDirectory + "FBX/2013.3/include/fbxsdk",
				}
			);

		string FBxLibPath = UEBuildConfiguration.UEThirdPartyDirectory
			+ "FBX/2013.3/lib/"
			+ "VS2012/" // VS2013 libs are not readily available for FBX, so let's just use the 2012 ones whilst we can.
			+ (Target.Platform == UnrealTargetPlatform.Win64 ? "x64" : "x86");

		bool bAllowDebugFBXLibraries = false;

		if (Target.Platform == UnrealTargetPlatform.Win64
			|| Target.Platform == UnrealTargetPlatform.Win32)
		{
			PublicLibraryPaths.Add(FBxLibPath);

			if (bAllowDebugFBXLibraries && Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT)
			{
				PublicAdditionalLibraries.Add("fbxsdk-2013.3d.lib");
				Definitions.Add("FBXSDK_SHARED"); 
			}
			else
			{
				PublicAdditionalLibraries.Add("fbxsdk-2013.3.lib");
				Definitions.Add("FBXSDK_SHARED"); 
			}
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			string LibDir = UEBuildConfiguration.UEThirdPartyDirectory + "FBX/2013.3/lib/mac/";

			if (bAllowDebugFBXLibraries && Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT)
			{
				PublicAdditionalLibraries.Add(LibDir + "libfbxsdk-2013.3-staticd.a");
			}
			else
			{
				PublicAdditionalLibraries.Add(LibDir + "libfbxsdk-2013.3-static.a");
			}
		}
	}
}

