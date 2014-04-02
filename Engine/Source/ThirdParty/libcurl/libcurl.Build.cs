// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class libcurl : ModuleRules
{
	public libcurl(TargetInfo Target)
	{
		Type = ModuleType.External;

		Definitions.Add("WITH_LIBCURL=1");
		string LibCurlPath = UEBuildConfiguration.UEThirdPartyDirectory + "libcurl/";
		if (Target.Platform == UnrealTargetPlatform.Linux)
        {
			PublicIncludePaths.Add(LibCurlPath + "include");
			PublicLibraryPaths.Add(LibCurlPath + "lib/Linux");
            PublicAdditionalLibraries.Add("curl");
            PublicAdditionalLibraries.Add("crypto");
            PublicAdditionalLibraries.Add("ssl");
            PublicAdditionalLibraries.Add("dl");
        }
		else if (Target.Platform == UnrealTargetPlatform.Win32 ||
				 Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicIncludePaths.Add(LibCurlPath + "include/Windows");

			string LibCurlLibPath = LibCurlPath + "lib/";
			LibCurlLibPath += (Target.Platform == UnrealTargetPlatform.Win64) ? "Win64/" : "Win32/";
			LibCurlLibPath += "VS" + WindowsPlatform.GetVisualStudioCompilerVersionName();
			PublicLibraryPaths.Add(LibCurlLibPath);

			PublicAdditionalLibraries.Add("libcurl_a.lib");
			Definitions.Add("CURL_STATICLIB=1");
		}
	}
}
