// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;

public class SDL : ModuleRules
{
	public SDL(TargetInfo Target)
	{
		Type = ModuleType.External;

		if (Target.Platform == UnrealTargetPlatform.HTML5 && Target.Architecture == "-win32")
		{
			PublicIncludePaths.Add(UEBuildConfiguration.UEThirdPartyDirectory + "SDL");
			PublicAdditionalLibraries.Add(UEBuildConfiguration.UEThirdPartyDirectory + "SDL/lib/x86/SDL.lib");
			PublicDelayLoadDLLs.Add("SDL.dll");
		}
	}
}
