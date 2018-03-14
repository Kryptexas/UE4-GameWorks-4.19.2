// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.


#include "Sound/SoundEffectSubmix.h"

USoundEffectSubmixPreset::USoundEffectSubmixPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void FSoundEffectSubmix::ProcessAudio(FSoundEffectSubmixInputData& InData, FSoundEffectSubmixOutputData& OutData)
{
	bIsRunning = true;
	InData.PresetData = nullptr;

	Update();

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
