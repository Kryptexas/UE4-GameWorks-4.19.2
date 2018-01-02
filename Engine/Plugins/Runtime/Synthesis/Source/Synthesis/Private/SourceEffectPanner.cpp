// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "SourceEffects/SourceEffectPanner.h"
#include "DSP/Dsp.h"

void FSourceEffectPanner::Init(const FSoundEffectSourceInitData& InitData)
{
	bIsActive = true;
	PanValue = 0.0f;
}

void FSourceEffectPanner::OnPresetChanged()
{
	GET_EFFECT_SETTINGS(SourceEffectPanner);

	// Normalize the panning value to be between 0.0 and 1.0
	PanValue = 0.5f * (1.0f - Settings.Pan);

	// Convert to radians between 0.0 and PI/2
	PanValue *= 0.5f * PI;
}

void FSourceEffectPanner::ProcessAudio(const FSoundEffectSourceInputData& InData, FSoundEffectSourceOutputData& OutData)
{
	const int32 NumChannels = InData.AudioFrame.Num();

	// If only 1 channel, do a simple passthrough
	if (NumChannels == 1)
	{
		OutData.AudioFrame[0] = InData.AudioFrame[0];
	}
	else
	{
		// Use the "cosine" equal power panning law to compute a smooth pan based off our parameter
		float PanGains[2];
		FMath::SinCos(&PanGains[0], &PanGains[1], PanValue);

		// Then simply scale the pan value output with the channel inputs. 
		for (int32 i = 0; i < NumChannels; ++i)
		{
			// Clamp this to be between 0.0 and 1.0 since SinCos is fast and may have values greater than 1.0 or less than 0.0
			float PanGain = FMath::Clamp(PanGains[i], 0.0f, 1.0f);

			// Simply scale the input sample with the pan value 
			OutData.AudioFrame[i] = PanGain * InData.AudioFrame[i];
		}
	}
}

void USourceEffectPannerPreset::SetSettings(const FSourceEffectPannerSettings& InSettings)
{
	UpdateSettings(InSettings);
}