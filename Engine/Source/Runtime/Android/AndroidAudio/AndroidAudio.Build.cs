// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AndroidAudio : ModuleRules
{
	public AndroidAudio(TargetInfo Target)
	{
		PrivateIncludePathModuleNames.Add("TargetPlatform");

        PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
		});

		AddEngineThirdPartyPrivateStaticDependencies(Target,
			"UEOgg",
			"Vorbis",
			"VorbisFile"
		);
	}
}
