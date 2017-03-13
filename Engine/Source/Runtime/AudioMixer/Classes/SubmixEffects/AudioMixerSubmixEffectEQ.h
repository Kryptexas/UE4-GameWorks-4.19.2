// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DSP/EQ.h"
#include "Sound/SoundEffectSubmix.h"
#include "AudioMixerSubmixEffectEQ.generated.h"

USTRUCT()
struct FSubmixEffectSubmixEQSettings
{
	GENERATED_USTRUCT_BODY()

	// not applicable 
};

class AUDIOMIXER_API FSubmixEffectSubmixEQ : public FSoundEffectSubmix
{
public:
		// Called on an audio effect at initialization on main thread before audio processing begins.
	virtual void Init(const FSoundEffectSubmixInitData& InSampleRate) override;

	// Process the input block of audio. Called on audio thread.
	virtual void OnProcessAudio(const FSoundEffectSubmixInputData& InData, FSoundEffectSubmixOutputData& OutData) override;

	// Sets the effect parameters
	void SetEffectParameters(const FAudioEQEffect& InEQEffectParameters);

	virtual void SetPreset(USoundEffectSubmixPreset* InPreset) override {};

protected:
	void UpdateParameters();

	struct FEQBandSettings
	{
		bool bEnabled;
		float Frequency;
		float Bandwidth;
		float GainDB;
	};

	struct FEQSettings
	{
		FEQBandSettings Bands[4];
	};

	// The EQ effect
	Audio::FEqualizer Equalizer;
	Audio::TParams<FEQSettings> Params;

	// Copy of EQ settings on render thread
	FEQSettings EQSettings;
};

UCLASS()
class AUDIOMIXER_API USubmixEffectSubmixEQPreset : public USoundEffectSubmixPreset
{
	GENERATED_BODY()

public:

	EFFECT_PRESET_METHODS_NO_ASSET_ACTIONS(SubmixEffectSubmixEQ)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SubmixEffectPreset)
	FSubmixEffectSubmixEQSettings Settings;
};