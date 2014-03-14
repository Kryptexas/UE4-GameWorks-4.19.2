// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;
using System.IO;
using System.Diagnostics;
public class OpenAL : ModuleRules
{
	public OpenAL(TargetInfo Target)
	{
		Type = ModuleType.External;
		string version = "1.15.1";
		if (Target.Platform == UnrealTargetPlatform.HTML5 ) 
		{
            string openAL = UEBuildConfiguration.UEThirdPartyDirectory + "OpenAL/" + version + "/include";
            PublicIncludePaths.Add(openAL);
			if (Target.Architecture == "-win32")
			{
				// add libs for OpenAL32 
				PublicAdditionalLibraries.Add(UEBuildConfiguration.UEThirdPartyDirectory + "OpenAL/" + version + "/lib/Win32/libOpenAL32.dll.a");
			}
		}
	}
}
