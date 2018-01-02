// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AudioCaptureEditor : ModuleRules
{
	public AudioCaptureEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"UnrealEd",
				"SequenceRecorder",
                "AudioEditor",
                "AudioCapture",
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
                "AudioCaptureEditor/Private",
                "AudioCapture/Private"
			}
		);

        if (Target.Platform == UnrealTargetPlatform.Win32 ||
            Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicDefinitions.Add("WITH_RTAUDIO=1");
            PublicDefinitions.Add("WITH_AUDIOCAPTURE=1");

            // Allow us to use direct sound
            AddEngineThirdPartyPrivateStaticDependencies(Target, "DirectSound");
        }
	}
}
