// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AudioFormatADPCM : ModuleRules
{
	public AudioFormatADPCM(TargetInfo Target)
	{
		PrivateIncludePathModuleNames.Add("TargetPlatform");
		PrivateDependencyModuleNames.Add("Core");
		if (UEBuildConfiguration.bCompileAgainstEngine)
		{
			PrivateDependencyModuleNames.Add("Engine");
		}
	}
}
