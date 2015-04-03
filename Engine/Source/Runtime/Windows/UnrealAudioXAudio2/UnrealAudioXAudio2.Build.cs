// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UnrealAudioXAudio2 : ModuleRules
{
	public UnrealAudioXAudio2(TargetInfo Target)
	{
		PrivateIncludePaths.AddRange(
			new string[] {
				"Runtime/UnrealAudio/Private",
			}
)		;

		PrivateIncludePathModuleNames.Add("TargetPlatform");

		PrivateDependencyModuleNames.AddRange(
		new string[] {
				"Core",
				"CoreUObject",
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"UnrealAudio"
			}
		);

		AddThirdPartyPrivateStaticDependencies(Target, 
			"DX11Audio"
		);
	}
}
