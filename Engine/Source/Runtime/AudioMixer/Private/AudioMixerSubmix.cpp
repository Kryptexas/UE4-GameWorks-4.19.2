// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AudioMixerSubmix.h"
#include "AudioMixerDevice.h"
#include "AudioMixerSourceVoice.h"
#include "Sound/SoundSubmix.h"
#include "Sound/SoundEffectSubmix.h"

namespace Audio
{
	// Unique IDs for mixer submixe's
	static uint32 GSubmixMixerIDs = 0;

	FMixerSubmix::FMixerSubmix(USoundSubmix* InSoundSubmix, FMixerDevice* InMixerDevice)
		: Id(GSubmixMixerIDs++)
		, ParentSubmix(nullptr)
		, MixerDevice(InMixerDevice)
		, WetLevel(0.5f)
	{
		if (InSoundSubmix != nullptr)
		{
			// Setup the parent and child submix instances

			ParentSubmix = InMixerDevice->GetSubmixInstance(InSoundSubmix->ParentSubmix);

			for (int32 i = 0; i < InSoundSubmix->ChildSubmixes.Num(); ++i)
			{
				FMixerSubmixPtr ChildSubmix = InMixerDevice->GetSubmixInstance(InSoundSubmix->ChildSubmixes[i]);
				ChildSubmixes.Add(ChildSubmix->GetId(), ChildSubmix);
			}

			// Loop through the submix's presets and make new instances of effects in the same order as the presets
			EffectSubmixChain.Reset();

			for (USoundEffectSubmixPreset* EffectPreset : InSoundSubmix->SubmixEffectChain)
			{
				if (EffectPreset)
				{
					// Create a new effect instance using the preset
					FSoundEffectSubmix* SubmixEffect = static_cast<FSoundEffectSubmix*>(EffectPreset->CreateNewEffect());

					FSoundEffectSubmixInitData InitData;
					InitData.NumOutputChannels = InMixerDevice->GetNumDeviceChannels();
					InitData.SampleRate = InMixerDevice->GetSampleRate();
					InitData.PresetSettings = nullptr;

					// Now set the preset
					SubmixEffect->Init(InitData);
					SubmixEffect->SetPreset(EffectPreset);
					SubmixEffect->Enable();

					// Add the effect to this submix's chain
					EffectSubmixChain.Add(SubmixEffect);
				}
			}
		}
	}

	FMixerSubmix::~FMixerSubmix()
	{
	}

	void FMixerSubmix::SetParentSubmix(TSharedPtr<FMixerSubmix, ESPMode::ThreadSafe> Submix)
	{
		ParentSubmix = Submix;
	}

	void FMixerSubmix::AddChildSubmix(TSharedPtr<FMixerSubmix, ESPMode::ThreadSafe> Submix)
	{
		ChildSubmixes.Add(Submix->GetId(), Submix);
	}

	TSharedPtr<FMixerSubmix, ESPMode::ThreadSafe> FMixerSubmix::GetParentSubmix()
	{
		return ParentSubmix;
	}

	int32 FMixerSubmix::GetNumSourceVoices() const
	{
		return MixerSourceVoices.Num();
	}

	int32 FMixerSubmix::GetNumEffects() const
	{
		return EffectSubmixChain.Num();
	}

	void FMixerSubmix::AddOrSetSourceVoice(FMixerSourceVoice* InSourceVoice, const float InDryLevel, const float InWetLevel)
	{
		AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

		FSubmixEffectSendInfo EffectInfo = { InDryLevel, InWetLevel };
		MixerSourceVoices.Add(InSourceVoice, EffectInfo);
	}

	void FMixerSubmix::RemoveSourceVoice(FMixerSourceVoice* InSourceVoice)
	{
		AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);
		int32 NumRemoved = MixerSourceVoices.Remove(InSourceVoice);
		AUDIO_MIXER_CHECK(NumRemoved == 1);
	}

	void FMixerSubmix::AddSoundEffectSubmix(FSoundEffectSubmix* InSubmixEffect)
	{
		EffectSubmixChain.Add(InSubmixEffect);
	}

	void FMixerSubmix::DownmixBuffer(const int32 InputChannelCount, const TArray<float>& InBuffer, const int32 DownMixChannelCount, TArray<float>& OutDownmixedBuffer)
	{
		// Retrieve ptr to the cached downmix channel map from the mixer device
		const float* DownmixChannelMap = MixerDevice->Get2DChannelMap(InputChannelCount, DownMixChannelCount);

		// Input and output frame count is going to be the same
		const int32 InputFrames = InBuffer.Num() / InputChannelCount;

		// Reset the passed in downmix scratch buffer
		OutDownmixedBuffer.Reset();
		OutDownmixedBuffer.AddZeroed(InputFrames * DownMixChannelCount);

		// Loop through the down mix map and perform the downmix operation
		int32 InputSampleIndex = 0;
		int32 DownMixedBufferIndex = 0;
		for (; InputSampleIndex < WetBuffer.Num();)
		{
			for (int32 DownMixChannel = 0; DownMixChannel < DownMixChannelCount; ++DownMixChannel)
			{
				for (int32 InChannel = 0; InChannel < InputChannelCount; ++InChannel)
				{
					const int32 ChannelMapIndex = DownMixChannelCount * InChannel + DownMixChannel;
					DownmixedBuffer[DownMixedBufferIndex + DownMixChannel] += WetBuffer[InputSampleIndex + InChannel] * DownmixChannelMap[ChannelMapIndex];
				}
			}

			InputSampleIndex += InputChannelCount;
			DownMixedBufferIndex += DownMixChannelCount;
		}
	}

	void FMixerSubmix::ProcessAudio(TArray<float>& OutAudioBuffer)
	{
		AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

		// Create a zero'd scratch buffer to get the audio from this submix's children
		const int32 NumSamples = OutAudioBuffer.Num();

		WetBuffer.Reset(NumSamples);
		WetBuffer.AddZeroed(NumSamples);

		DryBuffer.Reset(NumSamples);
		DryBuffer.AddZeroed(NumSamples);

		// First loop this submix's child submixes mixing in their output into this submix's dry/wet buffers.
		for (auto ChildSubmixEntry : ChildSubmixes)
		{
			TSharedPtr<FMixerSubmix, ESPMode::ThreadSafe> ChildSubmix = ChildSubmixEntry.Value;

			AUDIO_MIXER_CHECK(ChildSubmix.IsValid());

			ScratchBuffer.Reset(NumSamples);
			ScratchBuffer.AddZeroed(NumSamples);

			ChildSubmix->ProcessAudio(ScratchBuffer);

			// Get this child submix's wet level
			float ChildWetLevel = ChildSubmix->WetLevel;
			float ChildDryLevel = 1.0f - ChildWetLevel;

			// Mix the child's submix results into this submix's dry and wet levels
			for (int32 i = 0; i < NumSamples; ++i)
			{
				WetBuffer[i] += ChildDryLevel * ScratchBuffer[i];
				DryBuffer[i] += ChildDryLevel* ScratchBuffer[i];
			}
		}

		// Loop through this submix's sound sources
		for (auto MixerSourceVoiceIter : MixerSourceVoices)
		{
			const FMixerSourceVoice* MixerSourceVoice = MixerSourceVoiceIter.Key;
			const FSubmixEffectSendInfo& EffectInfo = MixerSourceVoiceIter.Value;

			MixerSourceVoice->MixOutputBuffers(DryBuffer, WetBuffer, EffectInfo.DryLevel, EffectInfo.WetLevel);
		}

		if (EffectSubmixChain.Num() > 0)
		{
			// Setup the input data buffer
			FSoundEffectSubmixInputData InputData;
			InputData.AudioClock = MixerDevice->GetAudioTime();

			// Compute the number of frames of audio. This will be independent of if we downmix our wet buffer.
			const int32 NumOutputChannels = MixerDevice->GetNumDeviceChannels();
			const int32 NumFrames = NumSamples / NumOutputChannels;
			InputData.NumFrames = NumFrames;

			FSoundEffectSubmixOutputData OutputData;
			OutputData.AudioBuffer = &ScratchBuffer;
			OutputData.NumChannels = NumOutputChannels;

			for (FSoundEffectSubmix* SubmixEffect : EffectSubmixChain)
			{
				// Reset the output scratch buffer
				ScratchBuffer.Reset(NumSamples);
				ScratchBuffer.AddZeroed(NumSamples);

				// Check to see if we need to downmix our audio before sending to the submix effect
				const uint32 ChannelCountOverride = SubmixEffect->GetDesiredInputChannelCountOverride();
				const int32 NumDeviceChannels = MixerDevice->GetNumDeviceChannels();
				if (ChannelCountOverride < (uint32)NumDeviceChannels)
				{
					// Perform the downmix operation with the downmixed scratch buffer
					DownmixBuffer(NumDeviceChannels, WetBuffer, ChannelCountOverride, DownmixedBuffer);

					InputData.NumChannels = ChannelCountOverride;
					InputData.AudioBuffer = &DownmixedBuffer;
					SubmixEffect->ProcessAudio(InputData, OutputData);
				}
				else
				{
					// If we're not downmixing, then just pass in the current wet buffer and our channel count is the same as the output channel count
					InputData.NumChannels = NumOutputChannels;
					InputData.AudioBuffer = &WetBuffer;
					SubmixEffect->ProcessAudio(InputData, OutputData);
				}

				// Copy the output audio buffer to the old input data to be reused for next submix effect
				WetBuffer = ScratchBuffer;
			}
		}

		// Mix the dry and wet buffers together for the final output
		for (int32 i = 0; i < NumSamples; ++i)
		{
			OutAudioBuffer[i] = (DryBuffer[i] + WetBuffer[i]);
		}
	}


	void FMixerSubmix::SetOutputWetLevel(const float InWetLevel)
	{
		WetLevel = InWetLevel;
	}

	float FMixerSubmix::GetOutputWetLevel() const
	{
		return WetLevel;
	}

	int32 FMixerSubmix::GetSampleRate() const
	{
		return MixerDevice->GetDeviceSampleRate();
	}

	int32 FMixerSubmix::GetNumOutputChannels() const
	{
		return MixerDevice->GetNumDeviceChannels();
	}

	int32 FMixerSubmix::GetNumChainEffects() const
	{
		return EffectSubmixChain.Num();
	}

	FSoundEffectSubmix* FMixerSubmix::GetSubmixEffect(const int32 InIndex)
	{
		if (InIndex < EffectSubmixChain.Num())
		{
			return EffectSubmixChain[InIndex];
		}
		return nullptr;
	}

	void FMixerSubmix::Update()
	{
	}


}