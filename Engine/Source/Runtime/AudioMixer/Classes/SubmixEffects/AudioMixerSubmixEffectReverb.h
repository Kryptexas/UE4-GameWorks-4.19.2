// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DSP/Reverb.h"
#include "Sound/SoundEffectSubmix.h"
#include "AudioEffect.h"
#include "AudioMixerSubmixEffectReverb.generated.h"

// ========================================================================
// FSubmixEffectReverbSettings
// ========================================================================

USTRUCT()
struct AUDIOMIXER_API FSubmixEffectReverbSettings
{
	GENERATED_USTRUCT_BODY()

	// No settings to actually set for master reverb. 
	// These are set using UReverbEffect.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Reverb)
	UReverbEffect* ReverbEffect;
};

class AUDIOMIXER_API FSubmixEffectReverb : public FSoundEffectSubmix
{
public:
	// Called on an audio effect at initialization on main thread before audio processing begins.
	void Init(const FSoundEffectSubmixInitData& InSampleRate) override;
	
	// We want to receive downmixed submix audio to stereo input for the reverb effect
	uint32 GetDesiredInputChannelCountOverride() const override { return 2; }

	// Process the input block of audio. Called on audio thread.
	void OnProcessAudio(const FSoundEffectSubmixInputData& InData, FSoundEffectSubmixOutputData& OutData) override;

	// Sets the reverb effect parameters based from audio thread code
	void SetEffectParameters(const FAudioReverbEffect& InReverbEffectParameters);

private:
	void UpdateParameters();

	// The reverb effect
	Audio::FPlateReverb PlateReverb;

	// The reverb effect params
	Audio::TParams<Audio::FPlateReverbSettings> Params;

	// Curve which maps old reverb times to new decay value
	FRichCurve DecayCurve;
};

UCLASS()
class AUDIOMIXER_API USubmixEffectReverbPreset : public USoundEffectSubmixPreset
{
	GENERATED_BODY()

public:
	EFFECT_PRESET_METHODS_NO_ASSET_ACTIONS(SubmixEffectReverb)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SubmixEffectPreset)
	FSubmixEffectReverbSettings Settings;
};
