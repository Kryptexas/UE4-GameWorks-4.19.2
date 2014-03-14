// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class portmidi : ModuleRules
{
	public portmidi(TargetInfo Target)
	{
		Type = ModuleType.External;

		PublicIncludePaths.Add(UEBuildConfiguration.UEThirdPartyDirectory + "portmidi/include");

        if (Target.Platform == UnrealTargetPlatform.Win32)
        {
            PublicLibraryPaths.Add(UEBuildConfiguration.UEThirdPartyDirectory + "portmidi/lib/Win32");
            PublicAdditionalLibraries.Add("portmidi.lib");
        }
        else if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicLibraryPaths.Add(UEBuildConfiguration.UEThirdPartyDirectory + "portmidi/lib/Win64");
            PublicAdditionalLibraries.Add("portmidi_64.lib");
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            PublicAdditionalLibraries.Add(UEBuildConfiguration.UEThirdPartyDirectory + "portmidi/lib/Mac/libportmidi.a");
            PublicAdditionalFrameworks.Add("CoreMIDI");
        }
	}
}
