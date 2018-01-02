// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class IntelVTune : ModuleRules
{
	public IntelVTune(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		string IntelVTunePath = Target.UEThirdPartySourceDirectory + "IntelVTune/VTune-2017/";

		if ( (Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32) )
		{
			string PlatformName = (Target.Platform == UnrealTargetPlatform.Win64) ? "Win64" : "Win32";

			PublicSystemIncludePaths.Add(IntelVTunePath + "include/");

			PublicLibraryPaths.Add(IntelVTunePath + "lib/" + PlatformName);

			PublicAdditionalLibraries.Add("libittnotify.lib");
		}
	}
}