// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.


#include "Sound/SoundEffectSubmix.h"

USoundEffectSubmixPreset::USoundEffectSubmixPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void FSoundEffectSubmix::ProcessAudio(FSoundEffectSubmixInputData& InData, FSoundEffectSubmixOutputData& OutData)
{
	bIsRunning = true;
	InData.PresetData = nullptr;

	// Update the latest version of the effect settings on the audio thread. If there are new settings,
	// then RawPresetDataScratchOutputBuffer will have the last data.
	if (EffectSettingsQueue.Dequeue(RawPresetDataScratchOutputBuffer))
	{
		InData.PresetData = RawPresetDataScratchOutputBuffer.GetData();
	}

	// Only process the effect if the effect is active
	if (bIsActive)
	{
		OnProcessAudio(InData, OutData);
	}
	else
	{
		// otherwise, bypass the effect and move inputs to outputs
		OutData.AudioBuffer = MoveTemp(InData.AudioBuffer);
	}
}
