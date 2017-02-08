// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SubmixEffects/AudioMixerSubmixEffectReverb.h"
#include "AudioMixerEffectsManager.h"
#include "Audio.h"
#include "AudioMixer.h"

void FSubmixEffectReverb::Init(const FSoundEffectSubmixInitData& InitData)
{
	Audio::FPlateReverbSettings NewSettings;

	NewSettings.LateDelayMsec = 0.0f;
	NewSettings.LateGain = 0.0f;
	NewSettings.Bandwidth = 0.9f;
	NewSettings.Diffusion = 0.65f;
	NewSettings.Dampening = 0.3f;
	NewSettings.Decay = 0.2f;
	NewSettings.Density = 0.8f;
	NewSettings.Wetness = 1.0f;

	Params.SetParams(NewSettings);

	PlateReverb.Init(InitData.SampleRate);

	DecayCurve.AddKey(0.0f, 0.99f);
	DecayCurve.AddKey(2.0f, 0.5f);
	DecayCurve.AddKey(5.0f, 0.2f);
	DecayCurve.AddKey(10.0f, 0.1f);
	DecayCurve.AddKey(18.0f, 0.01f);
	DecayCurve.AddKey(19.0f, 0.002f);
	DecayCurve.AddKey(20.0f, 0.0001f);
}

void FSubmixEffectReverb::SetPreset(USoundEffectSubmixPreset* InPreset)
{
}

void FSubmixEffectReverb::OnProcessAudio(const FSoundEffectSubmixInputData& InData, FSoundEffectSubmixOutputData& OutData)
{
	check(InData.NumChannels == 2);
	if (OutData.NumChannels < 2)
	{
		// Not supported
		return;
	}

	SCOPE_CYCLE_COUNTER(STAT_AudioMixerMasterReverb);

	UpdateParameters();

	const float* AudioData = InData.AudioBuffer->GetData();
	float* OutAudioData = OutData.AudioBuffer->GetData();

	// If we're output stereo, no need to do any cross over
	if (OutData.NumChannels == 2)
	{
		for (int32 SampleIndex = 0; SampleIndex < InData.AudioBuffer->Num(); SampleIndex += OutData.NumChannels)
		{
			PlateReverb.ProcessAudioFrame(&AudioData[SampleIndex], InData.NumChannels, &OutAudioData[SampleIndex], OutData.NumChannels);
		}
	}
	// 5.1 or higher surround sound. Map stereo output to quad output
	else if(OutData.NumChannels > 5)
	{
		for (int32 InSampleIndex = 0, OutSampleIndex = 0; InSampleIndex < InData.AudioBuffer->Num(); InSampleIndex += InData.NumChannels, OutSampleIndex += OutData.NumChannels)
		{
			// Processed downmixed audio frame
			PlateReverb.ProcessAudioFrame(&AudioData[InSampleIndex], InData.NumChannels, &OutAudioData[OutSampleIndex], InData.NumChannels);

			// Now do a cross-over to the back-left/back-right speakers from the front-left and front-right
			
			// Using standard speaker map order map the right output to the BackLeft channel
			OutAudioData[OutSampleIndex + EAudioMixerChannel::BackRight] = OutAudioData[OutSampleIndex + EAudioMixerChannel::FrontLeft];
			OutAudioData[OutSampleIndex + EAudioMixerChannel::BackLeft] = OutAudioData[OutSampleIndex + EAudioMixerChannel::FrontRight];
		}
	}
}

void FSubmixEffectReverb::SetEffectParameters(const FAudioReverbEffect& InParams)
{
	Audio::FPlateReverbSettings NewSettings;

	NewSettings.EarlyReflections.Gain = FMath::GetMappedRangeValueClamped({ 0.0f, 3.16f }, { 0.0f, 1.0f }, InParams.ReflectionsGain);
	NewSettings.EarlyReflections.PreDelayMsec = FMath::GetMappedRangeValueClamped({ 0.0f, 0.3f }, { 0.0f, 300.0f }, InParams.ReflectionsDelay);
	NewSettings.EarlyReflections.Bandwidth = FMath::GetMappedRangeValueClamped({ 0.0f, 1.0f }, { 0.0f, 1.0f }, InParams.GainHF);

	NewSettings.LateDelayMsec = FMath::GetMappedRangeValueClamped({ 0.0f, 0.1f }, { 0.0f, 100.0f }, InParams.LateDelay);
	NewSettings.LateGain = FMath::GetMappedRangeValueClamped({ 0.0f, 1.0f }, { 0.0f, 1.0f }, InParams.Gain);
	NewSettings.Bandwidth = FMath::GetMappedRangeValueClamped({ 0.0f, 1.0f }, { 0.2f, 0.999f }, InParams.AirAbsorptionGainHF);
	NewSettings.Diffusion = FMath::GetMappedRangeValueClamped({ 0.0f, 1.0f }, { 0.0f, 1.0f }, InParams.Diffusion);
	NewSettings.Dampening = FMath::GetMappedRangeValueClamped({ 0.1f, 2.0f }, { 0.0f, 0.999f }, InParams.DecayHFRatio);
	NewSettings.Density = FMath::GetMappedRangeValueClamped({ 0.0f, 1.0f }, { 0.01f, 1.0f }, InParams.Density);
	NewSettings.Wetness = FMath::GetMappedRangeValueClamped({ 0.0f, 10.0f }, { 0.0f, 10.0f }, InParams.Volume);

	// Use mapping function to get decay time in seconds to internal linear decay scale value
	const float DecayValue = DecayCurve.Eval(InParams.DecayTime);
	NewSettings.Decay = DecayValue;

	// Convert to db
	NewSettings.LateGain = Audio::ConvertToDecibels(NewSettings.LateGain);

	// Apply the settings the thread safe settings object
	Params.SetParams(NewSettings);
}

void FSubmixEffectReverb::UpdateParameters()
{
	Audio::FPlateReverbSettings NewSettings;
	if (Params.GetParams(&NewSettings))
	{
		PlateReverb.SetSettings(NewSettings);
	}
}
