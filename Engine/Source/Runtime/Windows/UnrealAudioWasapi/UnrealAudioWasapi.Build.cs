// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UnrealAudioWasapi : ModuleRules
{
	public UnrealAudioWasapi(TargetInfo Target)
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
	}
}
