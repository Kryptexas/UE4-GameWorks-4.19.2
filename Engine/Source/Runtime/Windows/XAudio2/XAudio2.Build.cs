// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class XAudio2 : ModuleRules
{
	public XAudio2(TargetInfo Target)
	{
		PrivateIncludePathModuleNames.Add("TargetPlatform");

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
			}
			);

		AddThirdPartyPrivateStaticDependencies(Target, 
			"DX11Audio",
			"UEOgg",
			"Vorbis",
			"VorbisFile"
			);
	}
}
