// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class OpenSubdiv : ModuleRules
{
	public OpenSubdiv(TargetInfo Target)
	{
		Type = ModuleType.External;

		// Compile and link with kissFFT
		string OpenSubdivPath = UEBuildConfiguration.UEThirdPartySourceDirectory + "OpenSubdiv/3.0.0-Alpha";

		PublicIncludePaths.Add( OpenSubdivPath + "/opensubdiv" );

		// @todo subdiv: Add other platforms and debug builds
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicLibraryPaths.Add(OpenSubdivPath + "/lib/RelWithDebInfo");
			PublicAdditionalLibraries.Add("osdCPU.lib");
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
		}
	}
}
