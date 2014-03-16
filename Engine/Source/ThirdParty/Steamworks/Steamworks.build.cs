// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Steamworks : ModuleRules
{
	public Steamworks(TargetInfo Target)
	{
		/** Mark the current version of the Steam SDK */
		string SteamVersion = "v128";
		Type = ModuleType.External;

		if ((Target.Platform == UnrealTargetPlatform.Win64) ||
			(Target.Platform == UnrealTargetPlatform.Win32))
		{
			PublicIncludePaths.Add(UEBuildConfiguration.UEThirdPartyDirectory + "Steamworks/Steam" + SteamVersion + "/sdk/public");

			string LibraryPath = UEBuildConfiguration.UEThirdPartyDirectory + "Steamworks/Steam" + SteamVersion + "/sdk/redistributable_bin/";
			string LibraryName = "steam_api";
			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				LibraryPath += "win64";
				LibraryName += "64";
			}
			PublicLibraryPaths.Add(LibraryPath);
			PublicAdditionalLibraries.Add(LibraryName + ".lib");
			PublicDelayLoadDLLs.Add(LibraryName + ".dll");
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			PublicIncludePaths.Add(UEBuildConfiguration.UEThirdPartyDirectory + "Steamworks/Steam" + SteamVersion + "/sdk/public");

			string LibraryPath = UEBuildConfiguration.UEThirdPartyDirectory + "Steamworks/Steam" + SteamVersion + "/sdk/redistributable_bin/osx32/libsteam_api.dylib";
			PublicDelayLoadDLLs.Add(LibraryPath);
		}
	}
}