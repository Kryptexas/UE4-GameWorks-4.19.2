// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FBX : ModuleRules
{
	public FBX(TargetInfo Target)
	{
		Type = ModuleType.External;

		PublicSystemIncludePaths.AddRange(
			new string[] {
					UEBuildConfiguration.UEThirdPartyDirectory + "FBX/2014.2.1/include",
					UEBuildConfiguration.UEThirdPartyDirectory + "FBX/2014.2.1/include/fbxsdk",
				}
			);


		if ( Target.Platform == UnrealTargetPlatform.Win64 )
		{
			string FBxLibPath = UEBuildConfiguration.UEThirdPartyDirectory + "FBX/2014.2.1/lib/vs2012/"; // VS2013 libs are not readily available for FBX, so let's just use the 2012 ones whilst we can.

			FBxLibPath += "x64/release/";
			PublicLibraryPaths.Add(FBxLibPath);

			PublicAdditionalLibraries.Add("libfbxsdk.lib");

			// We are using DLL versions of the FBX libraries
			Definitions.Add("FBXSDK_SHARED"); 
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			string LibDir = UEBuildConfiguration.UEThirdPartyDirectory + "FBX/2014.2.1/lib/mac/";
			PublicAdditionalLibraries.Add(LibDir + "libfbxsdk.dylib");
		}
	}
}

