// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.
// Not really Epic, but Freenode #ue4linux's RushPL and amigo 
using UnrealBuildTool;

public class LinuxNativeDialogs : ModuleRules
{
	public LinuxNativeDialogs(TargetInfo Target)
	{
		Type = ModuleType.External;

		string LNDPath = UEBuildConfiguration.UEThirdPartySourceDirectory + "LinuxNativeDialogs/UELinuxNativeDialogs/";
		string LNDLibPath = LNDPath + "lib/";

		if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			string CrossCompilingDir = (UnrealBuildTool.BuildHostPlatform.Current.Platform != UnrealTargetPlatform.Linux) ? "/NotForLicensees" : "";

            PublicLibraryPaths.Add(LNDLibPath + "Linux/" + Target.Architecture + CrossCompilingDir);
			PublicIncludePaths.Add(LNDPath + "include/");
			PublicAdditionalLibraries.Add("LND");
		}
	}
}
