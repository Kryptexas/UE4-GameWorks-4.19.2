// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DSP/Reverb.h"
#include "Sound/SoundEffectSubmix.h"
#include "AudioMixerSubmixEffectTest.generated.h"

// ========================================================================
// FSubmixEffectReverbSettings
// ========================================================================

USTRUCT()
struct AUDIOMIXER_API FSubmixEffectTestSettings
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = SoundEffect)
	float TestVolume;
};

// ========================================================================
// FSubmixEffectTest
// ========================================================================

class FSubmixEffectTest : public FSoundEffectSubmix
{
public:

	// Called on an audio effect at initialization on main thread before audio processing begins.
	void Init(const FSoundEffectSubmixInitData& InSampleRate) override;
	
	// Process the input block of audio. Called on audio thread.
	void OnProcessAudio(const FSoundEffectSubmixInputData& InData, FSoundEffectSubmixOutputData& OutData) override;

protected:
};

UCLASS()
class AUDIOMIXER_API USubmixEffectTestPreset : public USoundEffectSubmixPreset
{
	GENERATED_BODY()

public:
	EFFECT_PRESET_METHODS_NO_ASSET_ACTIONS(SubmixEffectTest)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SubmixEffectPreset)
	FSubmixEffectTestSettings Settings;
};
