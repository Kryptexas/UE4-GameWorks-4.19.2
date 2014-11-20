// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;

public class MikkTSpace : ModuleRules
{
	public MikkTSpace(TargetInfo Target)
	{
		Type = ModuleType.External;

		string MikkTSpacePath = UEBuildConfiguration.UEThirdPartySourceDirectory + "MikkTSpace/";

		PublicIncludePaths.Add(MikkTSpacePath + "inc/");

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicLibraryPaths.Add(MikkTSpacePath + "lib/x64/");
			PublicAdditionalLibraries.Add("MikkTSpace.lib");
		}
		else if (Target.Platform == UnrealTargetPlatform.Win32)
		{
			PublicLibraryPaths.Add(MikkTSpacePath + "lib/x86/");
			PublicAdditionalLibraries.Add("MikkTSpace.lib");
		}
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			PublicAdditionalLibraries.Add(MikkTSpacePath + "/lib/Linux/" + Target.Architecture + "/libMikkTSpace.a");
		}
	}
}
