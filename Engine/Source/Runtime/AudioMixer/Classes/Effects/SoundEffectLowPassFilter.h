// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Sound/SoundEffectSource.h"
#include "SoundEffectLowPassFilter.generated.h"

// ========================================================================
// FSoundEffectLowPassFilterSettings
// ========================================================================

USTRUCT()
struct AUDIOMIXER_API FSoundEffectLowPassFilterSettings
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = SoundEffect)
	float CutoffFrequency;

	UPROPERTY(EditAnywhere, Category = SoundEffect)
	float Q;
};

// ========================================================================
// USoundEffectLowPassFilter
// ========================================================================

class AUDIOMIXER_API FSoundEffectLowPassFilter : public FSoundEffectSource
{
public:
	// Called on an audio effect at initialization on main thread before audio processing begins.
	void Init(const FSoundEffectSourceInitData& InSampleRate) override {}
	
	// Process the input block of audio. Called on audio thread.
	void OnProcessAudio(const FSoundEffectSourceInputData& InData, FSoundEffectSourceOutputData& OutData) override {}
private:
};

UCLASS()
class AUDIOMIXER_API USoundEffectLowPassFilterPreset : public USoundEffectSourcePreset
{
	GENERATED_BODY()

public:
	EFFECT_PRESET_METHODS_NO_ASSET_ACTIONS(SoundEffectLowPassFilter)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SourceEffectPreset)
	FSoundEffectLowPassFilterSettings Settings;
};
