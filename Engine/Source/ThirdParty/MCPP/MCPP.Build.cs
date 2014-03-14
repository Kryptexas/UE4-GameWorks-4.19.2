// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MCPP : ModuleRules
{
	public MCPP(TargetInfo Target)
	{
		Type = ModuleType.External;

		PublicIncludePaths.Add(UEBuildConfiguration.UEThirdPartyDirectory + "MCPP/mcpp-2.7.2/inc");

		string LibPath = UEBuildConfiguration.UEThirdPartyDirectory + "MCPP/mcpp-2.7.2/lib/";

		if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            LibPath += ("Win64/VS" + WindowsPlatform.GetVisualStudioCompilerVersionName());
			PublicLibraryPaths.Add(LibPath);
			PublicAdditionalLibraries.Add("mcpp_64.lib");
		}
		else if (Target.Platform == UnrealTargetPlatform.Win32)
        {
            LibPath += ("Win32/VS" + WindowsPlatform.GetVisualStudioCompilerVersionName());
			PublicLibraryPaths.Add(LibPath);
			PublicAdditionalLibraries.Add("mcpp.lib");
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			PublicAdditionalLibraries.Add(LibPath + "Mac/libmcpp.a");
		}
	}
}

