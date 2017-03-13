// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.


#include "SubmixEffects/AudioMixerSubmixEffectEQ.h"
#include "Misc/ScopeLock.h"
#include "AudioMixer.h"

void FSubmixEffectSubmixEQ::Init(const FSoundEffectSubmixInitData& InitData)
{
	Equalizer.Init(InitData.SampleRate, 4, InitData.NumOutputChannels);
}

void FSubmixEffectSubmixEQ::OnProcessAudio(const FSoundEffectSubmixInputData& InData, FSoundEffectSubmixOutputData& OutData)
{
	SCOPE_CYCLE_COUNTER(STAT_AudioMixerMasterEQ);

	UpdateParameters();

	const float* AudioData = InData.AudioBuffer->GetData();
	float* OutAudioData = OutData.AudioBuffer->GetData();

	for (int32 SampleIndex = 0; SampleIndex < InData.AudioBuffer->Num(); SampleIndex += OutData.NumChannels)
	{
		Equalizer.ProcessAudioFrame(&AudioData[SampleIndex], &OutAudioData[SampleIndex]);
	}
}

static float GetClampedGain(const float InGain)
{
	// These are clamped to match XAudio2 FXEQ_MIN_GAIN and FXEQ_MAX_GAIN
	return FMath::Clamp(InGain, 0.001f, 7.94f);
}

static float GetClampedBandwidth(const float InBandwidth)
{
	// These are clamped to match XAudio2 FXEQ_MIN_BANDWIDTH and FXEQ_MAX_BANDWIDTH
	return FMath::Clamp(InBandwidth, 0.1f, 2.0f);
}

static float GetClampedFrequency(const float InFrequency)
{
	// These are clamped to match XAudio2 FXEQ_MIN_FREQUENCY_CENTER and FXEQ_MAX_FREQUENCY_CENTER
	return FMath::Clamp(InFrequency, 20.0f, 20000.0f);
}

void FSubmixEffectSubmixEQ::SetEffectParameters(const FAudioEQEffect& InEQEffectParameters)
{
	FEQSettings NewSettings;

	NewSettings.Bands[0].bEnabled = true;
	NewSettings.Bands[0].Frequency = GetClampedFrequency(InEQEffectParameters.FrequencyCenter0);
	NewSettings.Bands[0].GainDB= Audio::ConvertToDecibels(GetClampedGain(InEQEffectParameters.Gain0));
	NewSettings.Bands[0].Bandwidth = GetClampedBandwidth(InEQEffectParameters.Bandwidth0);

	NewSettings.Bands[1].bEnabled = true;
	NewSettings.Bands[1].Frequency = GetClampedFrequency(InEQEffectParameters.FrequencyCenter1);
	NewSettings.Bands[1].GainDB = Audio::ConvertToDecibels(GetClampedGain(InEQEffectParameters.Gain1));
	NewSettings.Bands[1].Bandwidth = GetClampedBandwidth(InEQEffectParameters.Bandwidth1);

	NewSettings.Bands[2].bEnabled = true;
	NewSettings.Bands[2].Frequency = GetClampedFrequency(InEQEffectParameters.FrequencyCenter2);
	NewSettings.Bands[2].GainDB = Audio::ConvertToDecibels(GetClampedGain(InEQEffectParameters.Gain2));
	NewSettings.Bands[2].Bandwidth = GetClampedBandwidth(InEQEffectParameters.Bandwidth2);

	NewSettings.Bands[3].bEnabled = true;
	NewSettings.Bands[3].Frequency = GetClampedFrequency(InEQEffectParameters.FrequencyCenter3);
	NewSettings.Bands[3].GainDB = Audio::ConvertToDecibels(GetClampedGain(InEQEffectParameters.Gain3));
	NewSettings.Bands[3].Bandwidth = GetClampedBandwidth(InEQEffectParameters.Bandwidth3);

	Params.SetParams(NewSettings);
}

void FSubmixEffectSubmixEQ::UpdateParameters()
{
	FEQSettings NewSettings;
	if (Params.GetParams(&NewSettings))
	{
		for (int32 Band = 0; Band < 4; ++Band)
		{
			Equalizer.SetBandParams(Band, NewSettings.Bands[Band].Frequency, NewSettings.Bands[Band].Bandwidth, NewSettings.Bands[Band].GainDB);
		}
	}
}
