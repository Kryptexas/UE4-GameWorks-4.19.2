// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;

public class SDL2 : ModuleRules
{
	public SDL2(TargetInfo Target)
	{
		Type = ModuleType.External;

		string SDL2Path = UEBuildConfiguration.UEThirdPartySourceDirectory + "SDL2/SDL2-2.0.3/";
		string SDL2LibPath = SDL2Path + "lib/";

		PublicIncludePaths.Add(SDL2Path + "include");

		if (Target.Platform == UnrealTargetPlatform.Linux)
		{
            PublicLibraryPaths.Add(SDL2LibPath + "Linux/" + Target.Architecture);
            PublicLibraryPaths.Add(UEBuildConfiguration.UEThirdPartySourceDirectory + "SDL2/DisplayServerExtensions/bin");
            PublicIncludePaths.Add(UEBuildConfiguration.UEThirdPartySourceDirectory + "SDL2/DisplayServerExtensions/include");
            PublicAdditionalLibraries.Add("EpicSDL2");
            PublicAdditionalLibraries.Add("dsext");
            PublicAdditionalLibraries.Add("dl");
		}
	}
}
