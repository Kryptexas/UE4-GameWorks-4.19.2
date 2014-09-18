// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;

public class SDL2 : ModuleRules
{
	public SDL2(TargetInfo Target)
	{
		Type = ModuleType.External;

		string SDL2Path = UEBuildConfiguration.UEThirdPartySourceDirectory + "SDL2/SDL-gui-backend/";
		string SDL2LibPath = SDL2Path + "lib/";

		PublicIncludePaths.Add(SDL2Path + "include");
		// assume SDL to be built with extensions
		Definitions.Add("SDL_WITH_EPIC_EXTENSIONS=1");

		if (Target.Platform == UnrealTargetPlatform.Linux)
		{
            if (Target.IsMonolithic)
            {
                PublicAdditionalLibraries.Add(SDL2LibPath + "Linux/" + Target.Architecture + "/libSDL2.a");
            }
            else
            {
                PublicAdditionalLibraries.Add(SDL2LibPath + "Linux/" + Target.Architecture + "/libSDL2_fPIC.a");
            }
		}
	}
}
