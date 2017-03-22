// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AudioMixerSourceManager.h"
#include "AudioMixerSource.h"
#include "AudioMixerDevice.h"
#include "AudioMixerSourceVoice.h"
#include "AudioMixerSubmix.h"
#include "IAudioExtensionPlugin.h"

#define ONEOVERSHORTMAX (3.0517578125e-5f) // 1/32768
#define ENVELOPE_TAIL_THRESHOLD (1.58489e-5f) // -96 dB

#define VALIDATE_SOURCE_MIXER_STATE 1

namespace Audio
{
	FSourceParam::FSourceParam(float InNumInterpFrames)
		: StartValue(0.0f)
		, EndValue(0.0f)
		, CurrentValue(0.0f)
		, NumInterpFrames(InNumInterpFrames)
		, Frame(0.0f)
		, bIsInit(true)
		, bIsDone(false)
	{
	}

	void FSourceParam::Reset()
	{
		bIsInit = true;
	}

	void FSourceParam::SetValue(float InValue)
	{
		if (bIsInit || NumInterpFrames == 0.0f)
		{
			bIsInit = false;
			CurrentValue = InValue;
			StartValue = InValue;
			EndValue = InValue;
			Frame = NumInterpFrames;
			if (NumInterpFrames == 0.0f)
			{
				bIsDone = true;
			}
			else
			{
				bIsDone = false;
			}
		}
		else
		{
			StartValue = CurrentValue;
			EndValue = InValue;
			Frame = 0.0f;
			bIsDone = false;
		}
	}

	float FSourceParam::Update()
	{
		if (bIsDone)
		{
			return EndValue;
		}
		else
		{
			float Alpha = Frame / NumInterpFrames;
			if (Alpha >= 1.0f)
			{
				bIsDone = true;
				CurrentValue = EndValue;
				return EndValue;
			}
			else
			{
				CurrentValue = FMath::Lerp(StartValue, EndValue, Alpha);
			}

			Frame += 1.0f;
			return CurrentValue;
		}
	}

	float FSourceParam::GetValue() const
	{
		return CurrentValue;
	}


	/*************************************************************************
	* FMixerSourceManager
	**************************************************************************/

	FMixerSourceManager::FMixerSourceManager(FMixerDevice* InMixerDevice)
		: MixerDevice(InMixerDevice)
		, NumActiveSources(0)
		, NumTotalSources(0)
		, NumSourceWorkers(4)
		, bInitialized(false)
	{
	}

	FMixerSourceManager::~FMixerSourceManager()
	{
		if (SourceWorkers.Num() > 0)
		{
			for (int32 i = 0; i < SourceWorkers.Num(); ++i)
			{
				delete SourceWorkers[i];
				SourceWorkers[i] = nullptr;
			}

			SourceWorkers.Reset();
		}
	}

	void FMixerSourceManager::Init(const FSourceManagerInitParams& InitParams)
	{
		AUDIO_MIXER_CHECK(InitParams.NumSources > 0);

		if (bInitialized || !MixerDevice)
		{
			return;
		}

		AUDIO_MIXER_CHECK(MixerDevice->GetSampleRate() > 0);

		NumTotalSources = InitParams.NumSources;

#if ENABLE_AUDIO_OUTPUT_DEBUGGING
		for (int32 i = 0; i < NumTotalSources; ++i)
		{
			DebugOutputGenerators.Add(FSineOsc(44100, 50 + i * 5, 0.5f));
		}
#endif

		MixerSources.Init(nullptr, NumTotalSources);
		BufferQueue.AddDefaulted(NumTotalSources);
		BufferQueueListener.Init(nullptr, NumTotalSources);
		CurrentPCMBuffer.Init(nullptr, NumTotalSources);
		CurrentAudioChunkNumFrames.AddDefaulted(NumTotalSources);
		SourceBuffer.AddDefaulted(NumTotalSources);
		AudioPluginOutputData.AddDefaulted(NumTotalSources);
		CurrentFrameValues.AddDefaulted(NumTotalSources);
		NextFrameValues.AddDefaulted(NumTotalSources);
		CurrentFrameAlpha.AddDefaulted(NumTotalSources);
		CurrentFrameIndex.AddDefaulted(NumTotalSources);
		NumFramesPlayed.AddDefaulted(NumTotalSources);

		PitchSourceParam.Init(FSourceParam(128), NumTotalSources);
		VolumeSourceParam.Init(FSourceParam(256), NumTotalSources);
		LPFCutoffFrequencyParam.Init(FSourceParam(128), NumTotalSources);
		ChannelMapParam.Init(FSourceChannelMap(256), NumTotalSources);
		SpatParams.AddDefaulted(NumTotalSources);
		ScratchChannelMap.AddDefaulted(NumTotalSources);
		LowPassFilters.AddDefaulted(NumTotalSources);
		SourceEffectChainId.Init(INDEX_NONE, NumTotalSources);
		SourceEffects.AddDefaulted(NumTotalSources);
		SourceEffectPresets.AddDefaulted(NumTotalSources);
		SourceEnvelopeFollower.Init(Audio::FEnvelopeFollower(AUDIO_SAMPLE_RATE, 10, 100, Audio::EPeakMode::Peak), NumTotalSources);
		SourceEnvelopeValue.AddDefaulted(NumTotalSources);
		bEffectTailsDone.AddDefaulted(NumTotalSources);
		SourceEffectInputData.AddDefaulted(NumTotalSources);
		SourceEffectOutputData.AddDefaulted(NumTotalSources);

		ReverbPluginOutputBuffer.AddDefaulted(NumTotalSources);
		PostEffectBuffers.AddDefaulted(NumTotalSources);
		OutputBuffer.AddDefaulted(NumTotalSources);
		bIs3D.AddDefaulted(NumTotalSources);
		bIsCenterChannelOnly.AddDefaulted(NumTotalSources);
		bIsActive.AddDefaulted(NumTotalSources);
		bIsPlaying.AddDefaulted(NumTotalSources);
		bIsPaused.AddDefaulted(NumTotalSources);
		bIsDone.AddDefaulted(NumTotalSources);
		bIsBusy.AddDefaulted(NumTotalSources);
		bUseHRTFSpatializer.AddDefaulted(NumTotalSources);
		bUseOcclusionPlugin.AddDefaulted(NumTotalSources);
		bUseReverbPlugin.AddDefaulted(NumTotalSources);
		bHasStarted.AddDefaulted(NumTotalSources);
		bIsDebugMode.AddDefaulted(NumTotalSources);
		DebugName.AddDefaulted(NumTotalSources);

		NumInputChannels.AddDefaulted(NumTotalSources);
		NumPostEffectChannels.AddDefaulted(NumTotalSources);
		NumInputFrames.AddDefaulted(NumTotalSources);

		GameThreadInfo.bIsBusy.AddDefaulted(NumTotalSources);
		GameThreadInfo.bIsDone.AddDefaulted(NumTotalSources);
		GameThreadInfo.bEffectTailsDone.AddDefaulted(NumTotalSources);
		GameThreadInfo.bNeedsSpeakerMap.AddDefaulted(NumTotalSources);
		GameThreadInfo.bIsDebugMode.AddDefaulted(NumTotalSources);
		GameThreadInfo.FreeSourceIndices.Reset(NumTotalSources);
		for (int32 i = NumTotalSources - 1; i >= 0; --i)
		{
			GameThreadInfo.FreeSourceIndices.Add(i);
		}

		// Initialize the source buffer memory usage to max source scratch buffers (num frames times max source channels)
		const int32 NumFrames = MixerDevice->GetNumOutputFrames();
		for (int32 SourceId = 0; SourceId < NumTotalSources; ++SourceId)
		{
			SourceBuffer[SourceId].Reset(NumFrames * 8);
			AudioPluginOutputData[SourceId].AudioBuffer.Reset(NumFrames * 2);
		}

		// Setup the source workers
		SourceWorkers.Reset();
		if (NumSourceWorkers > 0)
		{
			const int32 NumSourcesPerWorker = NumTotalSources / NumSourceWorkers;
			int32 StartId = 0;
			int32 EndId = 0;
			while (EndId < NumTotalSources)
			{
				EndId = FMath::Min(StartId + NumSourcesPerWorker, NumTotalSources);
				SourceWorkers.Add(new FAsyncTask<FAudioMixerSourceWorker>(this, StartId, EndId));
				StartId = EndId;
			}
		}
		AUDIO_MIXER_CHECK(SourceWorkers.Num() == NumSourceWorkers);

		bInitialized = true;
		bPumpQueue = false;
	}

	void FMixerSourceManager::Update()
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

#if VALIDATE_SOURCE_MIXER_STATE
		for (int32 i = 0; i < NumTotalSources; ++i)
		{
			if (!GameThreadInfo.bIsBusy[i])
			{
				// Make sure that our bIsFree and FreeSourceIndices are correct
				AUDIO_MIXER_CHECK(GameThreadInfo.FreeSourceIndices.Contains(i) == true);
			}
		}
#endif

		// Pump the source command queue from the game thread to make sure 
		// playsound calls, param updates, etc, all happen simultaneously
		bPumpQueue = true;
	}

	void FMixerSourceManager::ReleaseSource(const int32 SourceId)
	{
		AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

		AUDIO_MIXER_CHECK(SourceId < NumTotalSources);
		AUDIO_MIXER_CHECK(bInitialized);
		AUDIO_MIXER_CHECK(MixerSources[SourceId] != nullptr);

		AUDIO_MIXER_DEBUG_LOG(SourceId, TEXT("Is releasing"));

#if AUDIO_MIXER_ENABLE_DEBUG_MODE
		if (bIsDebugMode[SourceId])
		{
			DebugSoloSources.Remove(SourceId);
		}
#endif

		// Call OnRelease on the BufferQueueListener to give it a chance 
		// to release any resources it owns on the audio render thread
		if (BufferQueueListener[SourceId])
		{
			BufferQueueListener[SourceId]->OnRelease();
			BufferQueueListener[SourceId] = nullptr;
		}

		// Remove the mixer source from its submix sends
		TMap<uint32, FMixerSourceSubmixSend>& SubmixSends = MixerSources[SourceId]->GetSubmixSends();
		for (auto SubmixSendItem : SubmixSends)
		{
			SubmixSendItem.Value.Submix->RemoveSourceVoice(MixerSources[SourceId]);
		}

		// Delete the source effects
		SourceEffectChainId[SourceId] = INDEX_NONE;
		ResetSourceEffectChain(SourceId);

		SourceEnvelopeFollower[SourceId].Reset();
		bEffectTailsDone[SourceId] = true;

		// Delete the mixer source and null the slot
		delete MixerSources[SourceId];
		MixerSources[SourceId] = nullptr;

		// Reset all state and data
		PitchSourceParam[SourceId].Reset();
		VolumeSourceParam[SourceId].Reset();
		LPFCutoffFrequencyParam[SourceId].Reset();

		LowPassFilters[SourceId].Reset();
		ChannelMapParam[SourceId].Reset();
		BufferQueue[SourceId].Empty();
		CurrentPCMBuffer[SourceId] = nullptr;
		CurrentAudioChunkNumFrames[SourceId] = 0;
		SourceBuffer[SourceId].Reset();
		AudioPluginOutputData[SourceId].AudioBuffer.Reset();
		CurrentFrameValues[SourceId].Reset();
		NextFrameValues[SourceId].Reset();
		CurrentFrameAlpha[SourceId] = 0.0f;
		CurrentFrameIndex[SourceId] = 0;
		NumFramesPlayed[SourceId] = 0;
		PostEffectBuffers[SourceId] = nullptr;
		OutputBuffer[SourceId].Reset();
		bIs3D[SourceId] = false;
		bIsCenterChannelOnly[SourceId] = false;
		bIsActive[SourceId] = false;
		bIsPlaying[SourceId] = false;
		bIsDone[SourceId] = true;
		bIsPaused[SourceId] = false;
		bIsBusy[SourceId] = false;
		bUseHRTFSpatializer[SourceId] = false;
		bUseOcclusionPlugin[SourceId] = false;
		bUseReverbPlugin[SourceId] = false;
		bHasStarted[SourceId] = false;
		bIsDebugMode[SourceId] = false;
		DebugName[SourceId] = FString();
		NumInputChannels[SourceId] = 0;
		NumPostEffectChannels[SourceId] = 0;

		GameThreadInfo.bNeedsSpeakerMap[SourceId] = false;
	}

	void FMixerSourceManager::BuildSourceEffectChain(const int32 SourceId, FSoundEffectSourceInitData& InitData, const TArray<FSourceEffectChainEntry>& InSourceEffectChain)
	{
		// Create new source effects. The memory will be owned by the source manager.
		for (const FSourceEffectChainEntry& ChainEntry : InSourceEffectChain)
		{
			// Presets can have null entries
			if (!ChainEntry.Preset)
			{
				continue;
			}

			FSoundEffectSource* NewEffect = static_cast<FSoundEffectSource*>(ChainEntry.Preset->CreateNewEffect());
			NewEffect->RegisterWithPreset(ChainEntry.Preset);

			// Get this source effect presets unique id so instances can identify their originating preset object
			const uint32 PresetUniqueId = ChainEntry.Preset->GetUniqueID();
			InitData.ParentPresetUniqueId = PresetUniqueId;

			NewEffect->Init(InitData);
			NewEffect->SetPreset(ChainEntry.Preset);
			NewEffect->SetEnabled(!ChainEntry.bBypass);

			// Add the effect instance
			SourceEffects[SourceId].Add(NewEffect);

			// Add a slot entry for the preset so it can change while running. This will get sent to the running effect instance if the preset changes.
			SourceEffectPresets[SourceId].Add(nullptr);
		}
	}

	void FMixerSourceManager::ResetSourceEffectChain(const int32 SourceId)
	{
		for (int32 i = 0; i < SourceEffects[SourceId].Num(); ++i)
		{
			SourceEffects[SourceId][i]->UnregisterWithPreset();
			delete SourceEffects[SourceId][i];
			SourceEffects[SourceId][i] = nullptr;
		}
		SourceEffects[SourceId].Reset();

		for (int32 i = 0; i < SourceEffectPresets[SourceId].Num(); ++i)
		{
			SourceEffectPresets[SourceId][i] = nullptr;
		}
		SourceEffectPresets[SourceId].Reset();
	}

	bool FMixerSourceManager::GetFreeSourceId(int32& OutSourceId)
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		if (GameThreadInfo.FreeSourceIndices.Num())
		{
			OutSourceId = GameThreadInfo.FreeSourceIndices.Pop();

			AUDIO_MIXER_CHECK(OutSourceId < NumTotalSources);
			AUDIO_MIXER_CHECK(!GameThreadInfo.bIsBusy[OutSourceId]);

			AUDIO_MIXER_CHECK(!GameThreadInfo.bIsDebugMode[OutSourceId]);
			AUDIO_MIXER_CHECK(NumActiveSources < NumTotalSources);
			++NumActiveSources;

			GameThreadInfo.bIsBusy[OutSourceId] = true;
			return true;
		}
		AUDIO_MIXER_CHECK(false);
		return false;
	}

	int32 FMixerSourceManager::GetNumActiveSources() const
	{
		return NumActiveSources;
	}

	void FMixerSourceManager::InitSource(const int32 SourceId, const FMixerSourceVoiceInitParams& InitParams)
	{
		AUDIO_MIXER_CHECK(SourceId < NumTotalSources);
		AUDIO_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);
		AUDIO_MIXER_CHECK(!GameThreadInfo.bIsDebugMode[SourceId]);
		AUDIO_MIXER_CHECK(InitParams.BufferQueueListener != nullptr);
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

#if AUDIO_MIXER_ENABLE_DEBUG_MODE
		GameThreadInfo.bIsDebugMode[SourceId] = InitParams.bIsDebugMode;
#endif 

		AudioMixerThreadCommand([this, SourceId, InitParams]()
		{
			AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);
			AUDIO_MIXER_CHECK(InitParams.SourceVoice != nullptr);

			bIsPlaying[SourceId] = false;
			bIsPaused[SourceId] = false;
			bIsActive[SourceId] = true;
			bIsBusy[SourceId] = true;
			bIsDone[SourceId] = false;
			bUseHRTFSpatializer[SourceId] = InitParams.bUseHRTFSpatialization;

			BufferQueueListener[SourceId] = InitParams.BufferQueueListener;
			NumInputChannels[SourceId] = InitParams.NumInputChannels;
			NumInputFrames[SourceId] = InitParams.NumInputFrames;

			// Initialize the number of per-source LPF filters based on input channels
			LowPassFilters[SourceId].AddDefaulted(InitParams.NumInputChannels);

			// Setup the spatialization settings preset
			if (InitParams.SpatializationPluginSettings != nullptr)
			{
				MixerDevice->SpatializationPluginInterface->SetSpatializationSettings(SourceId, InitParams.SpatializationPluginSettings);
			}

			// Setup the occlusion settings preset
			if (InitParams.OcclusionPluginSettings != nullptr)
			{
				MixerDevice->OcclusionInterface->SetOcclusionSettings(SourceId, InitParams.OcclusionPluginSettings);

				bUseOcclusionPlugin[SourceId] = true;
			}

			// Setup the reverb settings preset
			if (InitParams.ReverbPluginSettings != nullptr)
			{
				MixerDevice->ReverbPluginInterface->SetReverbSettings(SourceId, InitParams.AudioComponentUserID, InitParams.ReverbPluginSettings);

				bUseReverbPlugin[SourceId] = true;
			}

			// Default all sounds to not consider effect chain tails when playing
			bEffectTailsDone[SourceId] = true;

			// Copy the source effect chain if the channel count is 1 or 2
			if (InitParams.NumInputChannels <= 2)
			{
				// If we're told to care about effect chain tails, then we're not allowed
				// to stop playing until the effect chain tails are finished
				bEffectTailsDone[SourceId] = !InitParams.bPlayEffectChainTails;

				FSoundEffectSourceInitData InitData;
				InitData.SampleRate = AUDIO_SAMPLE_RATE;
				InitData.NumSourceChannels = InitParams.NumInputChannels;
				InitData.AudioClock = MixerDevice->GetAudioTime();

				if (InitParams.NumInputFrames != INDEX_NONE)
				{
					InitData.SourceDuration = (float)InitParams.NumInputFrames / AUDIO_SAMPLE_RATE;
				}
				else
				{
					// Procedural sound waves have no known duration
					InitData.SourceDuration = (float)INDEX_NONE;
				}

				SourceEffectChainId[SourceId] = InitParams.SourceEffectChainId;
				BuildSourceEffectChain(SourceId, InitData, InitParams.SourceEffectChain);
			}

			CurrentFrameValues[SourceId].Init(0.0f, InitParams.NumInputChannels);
			NextFrameValues[SourceId].Init(0.0f, InitParams.NumInputChannels);

			AUDIO_MIXER_CHECK(MixerSources[SourceId] == nullptr);
			MixerSources[SourceId] = InitParams.SourceVoice;

			// Loop through the source's sends and add this source to those submixes with the send info
			for (int32 i = 0; i < InitParams.SubmixSends.Num(); ++i)
			{
				const FMixerSourceSubmixSend& MixerSourceSend = InitParams.SubmixSends[i];
				MixerSourceSend.Submix->AddOrSetSourceVoice(InitParams.SourceVoice, MixerSourceSend.SendLevel);
			}

#if AUDIO_MIXER_ENABLE_DEBUG_MODE
			AUDIO_MIXER_CHECK(!bIsDebugMode[SourceId]);
			bIsDebugMode[SourceId] = InitParams.bIsDebugMode;

			AUDIO_MIXER_CHECK(DebugName[SourceId].IsEmpty());
			DebugName[SourceId] = InitParams.DebugName;
#endif 

			AUDIO_MIXER_DEBUG_LOG(SourceId, TEXT("Is initializing"));
		});
	}

	void FMixerSourceManager::ReleaseSourceId(const int32 SourceId)
	{
		AUDIO_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		AUDIO_MIXER_CHECK(NumActiveSources > 0);
		--NumActiveSources;

		GameThreadInfo.bIsBusy[SourceId] = false;

#if AUDIO_MIXER_ENABLE_DEBUG_MODE
		GameThreadInfo.bIsDebugMode[SourceId] = false;
#endif

		GameThreadInfo.FreeSourceIndices.Push(SourceId);

		AUDIO_MIXER_CHECK(GameThreadInfo.FreeSourceIndices.Contains(SourceId));

		AudioMixerThreadCommand([this, SourceId]()
		{
			AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

			ReleaseSource(SourceId);
		});
	}

	void FMixerSourceManager::Play(const int32 SourceId)
	{
		AUDIO_MIXER_CHECK(SourceId < NumTotalSources);
		AUDIO_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		AudioMixerThreadCommand([this, SourceId]()
		{
			AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

			bIsPlaying[SourceId] = true;
			bIsPaused[SourceId] = false;
			bIsActive[SourceId] = true;

			AUDIO_MIXER_DEBUG_LOG(SourceId, TEXT("Is playing"));
		});
	}

	void FMixerSourceManager::Stop(const int32 SourceId)
	{
		AUDIO_MIXER_CHECK(SourceId < NumTotalSources);
		AUDIO_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		AudioMixerThreadCommand([this, SourceId]()
		{
			AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

			bIsPlaying[SourceId] = false;
			bIsPaused[SourceId] = false;
			bIsActive[SourceId] = false;

			AUDIO_MIXER_DEBUG_LOG(SourceId, TEXT("Is stopping"));
		});
	}

	void FMixerSourceManager::Pause(const int32 SourceId)
	{
		AUDIO_MIXER_CHECK(SourceId < NumTotalSources);
		AUDIO_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		AudioMixerThreadCommand([this, SourceId]()
		{
			AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

			bIsPaused[SourceId] = true;
			bIsActive[SourceId] = false;
		});
	}

	void FMixerSourceManager::SetPitch(const int32 SourceId, const float Pitch)
	{
		AUDIO_MIXER_CHECK(SourceId < NumTotalSources);
		AUDIO_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);

		AudioMixerThreadCommand([this, SourceId, Pitch]()
		{
			AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

			PitchSourceParam[SourceId].SetValue(Pitch);
		});
	}

	void FMixerSourceManager::SetVolume(const int32 SourceId, const float Volume)
	{
		AUDIO_MIXER_CHECK(SourceId < NumTotalSources);
		AUDIO_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		AudioMixerThreadCommand([this, SourceId, Volume]()
		{
			AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

			VolumeSourceParam[SourceId].SetValue(Volume);
		});
	}

	void FMixerSourceManager::SetSpatializationParams(const int32 SourceId, const FSpatializationParams& InParams)
	{
		AUDIO_MIXER_CHECK(SourceId < NumTotalSources);
		AUDIO_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		AudioMixerThreadCommand([this, SourceId, InParams]()
		{
			AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

			SpatParams[SourceId] = InParams;
		});
	}

	void FMixerSourceManager::SetChannelMap(const int32 SourceId, const TArray<float>& ChannelMap, const bool bInIs3D, const bool bInIsCenterChannelOnly)
	{
		AUDIO_MIXER_CHECK(SourceId < NumTotalSources);
		AUDIO_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		AudioMixerThreadCommand([this, SourceId, ChannelMap, bInIs3D, bInIsCenterChannelOnly]()
		{
			AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

			// Set whether or not this is a 3d channel map and if its center channel only. Used for reseting channel maps on device change.
			bIs3D[SourceId] = bInIs3D;
			bIsCenterChannelOnly[SourceId] = bInIsCenterChannelOnly;

			// Fix up the channel map in case device output count changed
			const int32 NumSourceChannels = bUseHRTFSpatializer[SourceId] ? 2 : NumInputChannels[SourceId];
			const int32 NumOutputChannels = MixerDevice->GetNumDeviceChannels();
			const int32 ChannelMapSize = NumSourceChannels * NumOutputChannels;

			// If this is true, then the device changed while the command was in-flight
			if (ChannelMap.Num() != ChannelMapSize)
			{
				TArray<float> NewChannelMap;

				// If 3d then just zero it out, we'll get another channel map shortly
				if (bInIs3D)
				{
					NewChannelMap.AddZeroed(ChannelMapSize);
					GameThreadInfo.bNeedsSpeakerMap[SourceId] = true;
				}
				// Otherwise, get an appropriate channel map for the new device configuration
				else
				{
					MixerDevice->Get2DChannelMap(NumSourceChannels, NumOutputChannels, bInIsCenterChannelOnly, NewChannelMap);
				}
				ChannelMapParam[SourceId].SetChannelMap(NewChannelMap);
			}
			else
			{
				GameThreadInfo.bNeedsSpeakerMap[SourceId] = false;
				ChannelMapParam[SourceId].SetChannelMap(ChannelMap);
			}

		});
	}

	void FMixerSourceManager::SetLPFFrequency(const int32 SourceId, const float InLPFFrequency)
	{
		AUDIO_MIXER_CHECK(SourceId < NumTotalSources);
		AUDIO_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		AudioMixerThreadCommand([this, SourceId, InLPFFrequency]()
		{
			AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

			float SampleRate = MixerDevice->GetSampleRate();
			AUDIO_MIXER_CHECK(SampleRate > 0.0f);

			const float NormalizedFrequency = 2.0f * InLPFFrequency / SampleRate;
			LPFCutoffFrequencyParam[SourceId].SetValue(NormalizedFrequency);
		});
	}

	void FMixerSourceManager::SubmitBuffer(const int32 SourceId, FMixerSourceBufferPtr InSourceVoiceBuffer, const bool bSubmitSynchronously)
	{
		AUDIO_MIXER_CHECK(SourceId < NumTotalSources);
		AUDIO_MIXER_CHECK(InSourceVoiceBuffer->AudioBytes <= (uint32)InSourceVoiceBuffer->AudioData.Num());

		// If we're submitting synchronously then don't use AudioMixerThreadCommand but submit directly
		if (bSubmitSynchronously)
		{
			BufferQueue[SourceId].Enqueue(InSourceVoiceBuffer);
		}
		else
		{
			// make sure we're on the game/audio thread
			AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

			// Use a thread command
			AudioMixerThreadCommand([this, SourceId, InSourceVoiceBuffer]()
			{
				AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

				BufferQueue[SourceId].Enqueue(InSourceVoiceBuffer);
			});
		}
	}


	void FMixerSourceManager::SetSubmixSendInfo(const int32 SourceId, FMixerSubmixPtr Submix, const float SendLevel)
	{
		AUDIO_MIXER_CHECK(SourceId < NumTotalSources);
		AUDIO_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		AudioMixerThreadCommand([this, SourceId, Submix, SendLevel]()
		{
			AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);
			Submix->AddOrSetSourceVoice(MixerSources[SourceId], SendLevel);
		});
	}

	int64 FMixerSourceManager::GetNumFramesPlayed(const int32 SourceId) const
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);
		return NumFramesPlayed[SourceId];
	}

	bool FMixerSourceManager::IsDone(const int32 SourceId) const
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);
		return GameThreadInfo.bIsDone[SourceId];
	}

	bool FMixerSourceManager::IsEffectTailsDone(const int32 SourceId) const
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);
		return GameThreadInfo.bEffectTailsDone[SourceId];
	}

	bool FMixerSourceManager::NeedsSpeakerMap(const int32 SourceId) const
	{
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);
		return GameThreadInfo.bNeedsSpeakerMap[SourceId];
	}

	void FMixerSourceManager::ReadSourceFrame(const int32 SourceId)
	{
		const int32 NumChannels = NumInputChannels[SourceId];

		// Check if the next frame index is out of range of the total number of frames we have in our current audio buffer
		bool bNextFrameOutOfRange = (CurrentFrameIndex[SourceId] + 1) >= CurrentAudioChunkNumFrames[SourceId];
		bool bCurrentFrameOutOfRange = CurrentFrameIndex[SourceId] >= CurrentAudioChunkNumFrames[SourceId];

		bool bReadCurrentFrame = true;

		// Check the boolean conditions that determine if we need to pop buffers from our queue (in PCMRT case) *OR* loop back (looping PCM data)
		while (bNextFrameOutOfRange || bCurrentFrameOutOfRange)
		{
			// If our current frame is in range, but next frame isn't, read the current frame now to avoid pops when transitioning between buffers
			if (bNextFrameOutOfRange && !bCurrentFrameOutOfRange)
			{
				// Don't need to read the current frame audio after reading new audio chunk
				bReadCurrentFrame = false;

				AUDIO_MIXER_CHECK(CurrentPCMBuffer[SourceId].IsValid());
				const int16* AudioData = (const int16*)CurrentPCMBuffer[SourceId]->AudioData.GetData();
				const int32 CurrentSampleIndex = CurrentFrameIndex[SourceId] * NumChannels;

				for (int32 Channel = 0; Channel < NumChannels; ++Channel)
				{
					CurrentFrameValues[SourceId][Channel] = (float)AudioData[CurrentSampleIndex + Channel] * ONEOVERSHORTMAX;
				}
			}

			// If this is our first PCM buffer, we don't need to do a callback to get more audio
			if (CurrentPCMBuffer[SourceId].IsValid())
			{
				if (CurrentPCMBuffer[SourceId]->LoopCount == Audio::LOOP_FOREVER && !CurrentPCMBuffer[SourceId]->bRealTimeBuffer)
				{
					AUDIO_MIXER_DEBUG_LOG(SourceId, TEXT("Hit Loop boundary, looping."));

					CurrentFrameIndex[SourceId] = FMath::Max(CurrentFrameIndex[SourceId] - CurrentAudioChunkNumFrames[SourceId], 0);
					break;
				}

				BufferQueueListener[SourceId]->OnSourceBufferEnd();
			}

			// If we have audio in our queue, we're still playing
			if (!BufferQueue[SourceId].IsEmpty())
			{
				FMixerSourceBufferPtr NewBufferPtr;
				BufferQueue[SourceId].Dequeue(NewBufferPtr);
				CurrentPCMBuffer[SourceId] = NewBufferPtr;

				AUDIO_MIXER_CHECK(MixerSources[SourceId]->NumBuffersQueued.GetValue() > 0);

				MixerSources[SourceId]->NumBuffersQueued.Decrement();

				CurrentAudioChunkNumFrames[SourceId] = CurrentPCMBuffer[SourceId]->AudioBytes / (NUM_BYTES_PER_SAMPLE * NumChannels);

				// Subtract the number of frames in the current buffer from our frame index.
				// Note: if this is the first time we're playing, CurrentFrameIndex will be 0
				if (bReadCurrentFrame)
				{
					CurrentFrameIndex[SourceId] = FMath::Max(CurrentFrameIndex[SourceId] - CurrentAudioChunkNumFrames[SourceId], 0);
				}
				else
				{
					// Since we're not reading the current frame, we allow the current frame index to be negative (NextFrameIndex will then be 0)
					// This prevents dropping a frame of audio on the buffer boundary
					CurrentFrameIndex[SourceId] = -1;
				}
			}
			else
			{
				bIsDone[SourceId] = true;
				return;
			}

			bNextFrameOutOfRange = (CurrentFrameIndex[SourceId] + 1) >= CurrentAudioChunkNumFrames[SourceId];
			bCurrentFrameOutOfRange = CurrentFrameIndex[SourceId] >= CurrentAudioChunkNumFrames[SourceId];
		}

		if (CurrentPCMBuffer[SourceId].IsValid())
		{
			// Grab the 16-bit PCM audio data (which could be a new audio chunk from previous ReadSourceFrame call)
			const int16* AudioData = (const int16*)CurrentPCMBuffer[SourceId]->AudioData.GetData();
			const int32 NextSampleIndex = (CurrentFrameIndex[SourceId] + 1)  * NumChannels;

			if (bReadCurrentFrame)
			{
				const int32 CurrentSampleIndex = CurrentFrameIndex[SourceId] * NumChannels;
				for (int32 Channel = 0; Channel < NumChannels; ++Channel)
				{
					CurrentFrameValues[SourceId][Channel] = (float)AudioData[CurrentSampleIndex + Channel] * ONEOVERSHORTMAX;
					NextFrameValues[SourceId][Channel] = (float)AudioData[NextSampleIndex + Channel] * ONEOVERSHORTMAX;
				}
			}
			else
			{
				for (int32 Channel = 0; Channel < NumChannels; ++Channel)
				{
					NextFrameValues[SourceId][Channel] = (float)AudioData[NextSampleIndex + Channel] * ONEOVERSHORTMAX;
				}
			}
		}
	}

	void FMixerSourceManager::ComputeSourceBuffersForIdRange(const int32 SourceIdStart, const int32 SourceIdEnd)
	{
		SCOPE_CYCLE_COUNTER(STAT_AudioMixerSourceBuffers);

		const int32 NumFrames = MixerDevice->GetNumOutputFrames();

		// Local variable used for sample values
		float SampleValue = 0.0f;

		for (int32 SourceId = SourceIdStart; SourceId < SourceIdEnd; ++SourceId)
		{
			SourceBuffer[SourceId].Reset();

			if (!bIsBusy[SourceId] || !bIsPlaying[SourceId] || bIsPaused[SourceId])
			{
				continue;
			}

			for (int32 Frame = 0; Frame < NumFrames; ++Frame)
			{
				// If the source is done, then we'll just write out 0s
				if (!bIsDone[SourceId])
				{
					// Whether or not we need to read another sample from the source buffers
					// If we haven't yet played any frames, then we will need to read the first source samples no matter what
					bool bReadNextSample = !bHasStarted[SourceId];

					// Reset that we've started generating audio
					bHasStarted[SourceId] = true;

					// Update the PrevFrameIndex value for the source based on alpha value
					while (CurrentFrameAlpha[SourceId] >= 1.0f)
					{
						// Our inter-frame alpha lerping value is causing us to read new source frames
						bReadNextSample = true;

						// Bump up the current frame index
						CurrentFrameIndex[SourceId]++;

						// Bump up the frames played -- this is tracking the total frames in source file played
						// CurrentFrameIndex can wrap for looping sounds so won't be accurate in that case
						NumFramesPlayed[SourceId]++;

						CurrentFrameAlpha[SourceId] -= 1.0f;
					}

					// If our alpha parameter caused us to jump to a new source frame, we need
					// read new samples into our prev and next frame sample data
					if (bReadNextSample)
					{
						ReadSourceFrame(SourceId);
					}
				}

				const int32 NumSourceChannels = NumInputChannels[SourceId];

				// If we've finished reading all buffer data, then just write out 0s
				if (bIsDone[SourceId])
				{
					for (int32 Channel = 0; Channel < NumSourceChannels; ++Channel)
					{
						SourceBuffer[SourceId].Add(0.0f);
					}
				}
				else
				{
					// Get the volume value of the source at this frame index
					for (int32 Channel = 0; Channel < NumSourceChannels; ++Channel)
					{
						const float CurrFrameValue = CurrentFrameValues[SourceId][Channel];
						const float NextFrameValue = NextFrameValues[SourceId][Channel];
						const float CurrentAlpha = CurrentFrameAlpha[SourceId];

						SampleValue = FMath::Lerp(CurrFrameValue, NextFrameValue, CurrentAlpha);
						SourceBuffer[SourceId].Add(SampleValue);
					}
					const float CurrentPitchScale = PitchSourceParam[SourceId].Update();

					CurrentFrameAlpha[SourceId] += CurrentPitchScale;
				}
			}
		}
	}

	void FMixerSourceManager::ComputePostSourceEffectBufferForIdRange(const int32 SourceIdStart, const int32 SourceIdEnd)
	{
		SCOPE_CYCLE_COUNTER(STAT_AudioMixerSourceEffectBuffers);
		const int32 NumFrames = MixerDevice->GetNumOutputFrames();

		const bool bIsDebugModeEnabled = DebugSoloSources.Num() > 0;

		for (int32 SourceId = SourceIdStart; SourceId < SourceIdEnd; ++SourceId)
		{
			if (!bIsBusy[SourceId] || !bIsPlaying[SourceId] || bIsPaused[SourceId])
			{
				continue;
			}

			// Get the source buffer
			TArray<float>& Buffer = SourceBuffer[SourceId];

			const int32 NumInputChans = NumInputChannels[SourceId];

			float CurrentVolumeValue = VolumeSourceParam[SourceId].GetValue();

			// Process the per-source LPF if the source hasn't actually been finished
			if (!bIsDone[SourceId])
			{

				for (int32 Frame = 0; Frame < NumFrames; ++Frame)
				{
					FSourceParam& LPFFrequencyParam = LPFCutoffFrequencyParam[SourceId];
					// Update the frequency at the block rate
					const float LPFFreq = LPFFrequencyParam.Update();

					// Update the volume
#if AUDIO_MIXER_ENABLE_DEBUG_MODE				
					CurrentVolumeValue = (bIsDebugModeEnabled && !bIsDebugMode[SourceId]) ? 0.0f : VolumeSourceParam[SourceId].Update();

#else
					CurrentVolumeValue = VolumeSourceParam[SourceId].Update();
#endif

					for (int32 Channel = 0; Channel < NumInputChans; ++Channel)
					{
						LowPassFilters[SourceId][Channel].SetFrequency(LPFFreq);

						// Process the source through the filter
						const int32 SampleIndex = NumInputChans * Frame + Channel;
						Buffer[SampleIndex] = CurrentVolumeValue * LowPassFilters[SourceId][Channel].ProcessAudio(Buffer[SampleIndex]);
					}
				}
			}

			const int32 NumSamples = Buffer.Num();

			// Now process the effect chain if it exists
			if (SourceEffects[SourceId].Num() > 0 && (!bIsDone[SourceId] || !bEffectTailsDone[SourceId]))
			{
				SourceEffectInputData[SourceId].CurrentVolume = CurrentVolumeValue;

				SourceEffectOutputData[SourceId].AudioFrame.Reset();
				SourceEffectOutputData[SourceId].AudioFrame.AddZeroed(NumInputChans);

				SourceEffectInputData[SourceId].AudioFrame.Reset();
				SourceEffectInputData[SourceId].AudioFrame.AddZeroed(NumInputChans);

				// Process the effect chain for this buffer per frame
				for (int32 Sample = 0; Sample < NumSamples; Sample += NumInputChans)
				{
					// Get the buffer input sample
					for (int32 Chan = 0; Chan < NumInputChans; ++Chan)
					{
						SourceEffectInputData[SourceId].AudioFrame[Chan] = Buffer[Sample + Chan];
					}

					for (int32 SourceEffectIndex = 0; SourceEffectIndex < SourceEffects[SourceId].Num(); ++SourceEffectIndex)
					{
						if (SourceEffects[SourceId][SourceEffectIndex]->IsActive())
						{
							SourceEffects[SourceId][SourceEffectIndex]->Update();

							SourceEffects[SourceId][SourceEffectIndex]->ProcessAudio(SourceEffectInputData[SourceId], SourceEffectOutputData[SourceId]);

							// Copy the output of the effect into the input so the next effect will get the outputs audio
							for (int32 Chan = 0; Chan < NumInputChans; ++Chan)
							{
								SourceEffectInputData[SourceId].AudioFrame[Chan] = SourceEffectOutputData[SourceId].AudioFrame[Chan];
							}
						}
					}

					// Copy audio frame back to the buffer
					for (int32 Chan = 0; Chan < NumInputChans; ++Chan)
					{
						Buffer[Sample + Chan] = SourceEffectInputData[SourceId].AudioFrame[Chan];
					}
				}
			}

			bool bReverbPluginUsed = false;
			if (bUseReverbPlugin[SourceId])
			{
				const FSpatializationParams* SourceSpatParams = &SpatParams[SourceId];

				// Move the audio buffer to the reverb plugin buffer
				AudioPluginInputData.SourceId = SourceId;
				AudioPluginInputData.AudioBuffer = &Buffer;
				AudioPluginInputData.SpatializationParams = SourceSpatParams;
				AudioPluginInputData.NumChannels = NumInputChans;
				AudioPluginOutputData[SourceId].AudioBuffer.Reset();
				AudioPluginOutputData[SourceId].AudioBuffer.AddZeroed(AudioPluginInputData.AudioBuffer->Num());

				MixerDevice->ReverbPluginInterface->ProcessSourceAudio(AudioPluginInputData, AudioPluginOutputData[SourceId]);

				// Make sure the buffer counts didn't change and are still the same size
				AUDIO_MIXER_CHECK(AudioPluginOutputData[SourceId].AudioBuffer.Num() == Buffer.Num());

				// Copy the reverb-processed data back to the source buffer
				ReverbPluginOutputBuffer[SourceId].Reset();
				ReverbPluginOutputBuffer[SourceId].Append(AudioPluginOutputData[SourceId].AudioBuffer);

				bReverbPluginUsed = true;
			}

			if (bUseOcclusionPlugin[SourceId])
			{
				const FSpatializationParams* SourceSpatParams = &SpatParams[SourceId];

				// Move the audio buffer to the occlusion plugin buffer
				AudioPluginInputData.SourceId = SourceId;
				AudioPluginInputData.AudioBuffer = &Buffer;
				AudioPluginInputData.SpatializationParams = SourceSpatParams;
				AudioPluginInputData.NumChannels = NumInputChans;

				AudioPluginOutputData[SourceId].AudioBuffer.Reset();
				AudioPluginOutputData[SourceId].AudioBuffer.AddZeroed(AudioPluginInputData.AudioBuffer->Num());

				MixerDevice->OcclusionInterface->ProcessAudio(AudioPluginInputData, AudioPluginOutputData[SourceId]);

				// Make sure the buffer counts didn't change and are still the same size
				AUDIO_MIXER_CHECK(AudioPluginOutputData[SourceId].AudioBuffer.Num() == Buffer.Num());

				// Copy the occlusion-processed data back to the source buffer and mix with the reverb plugin output buffer
				if (bReverbPluginUsed)
				{
					for (int32 i = 0; i < Buffer.Num(); ++i)
					{
						Buffer[i] = ReverbPluginOutputBuffer[SourceId][i] + AudioPluginOutputData[SourceId].AudioBuffer[i];
					}
				}
				else
				{
					FMemory::Memcpy(Buffer.GetData(), AudioPluginOutputData[SourceId].AudioBuffer.GetData(), sizeof(float) * Buffer.Num());
				}
			}
			else if (bReverbPluginUsed)
			{
				for (int32 i = 0; i < Buffer.Num(); ++i)
				{
					Buffer[i] += ReverbPluginOutputBuffer[SourceId][i];
				}
			}

			// Compute the source envelope
			float AverageSampleValue;
			for (int32 Sample = 0; Sample < NumSamples;)
			{
				AverageSampleValue = 0.0f;
				for (int32 Chan = 0; Chan < NumInputChans; ++Chan)
				{
					AverageSampleValue += Buffer[Sample++];
				}
				AverageSampleValue /= NumInputChans;
				SourceEnvelopeFollower[SourceId].ProcessAudio(AverageSampleValue);
			}

			// Copy the current value of the envelope follower (block-rate value)
			SourceEnvelopeValue[SourceId] = SourceEnvelopeFollower[SourceId].GetCurrentValue();

			bEffectTailsDone[SourceId] = bEffectTailsDone[SourceId] || SourceEnvelopeValue[SourceId] < ENVELOPE_TAIL_THRESHOLD;

			// Check the source effect tails condition
			if (bIsDone[SourceId] && bEffectTailsDone[SourceId])
			{
				// If we're done and our tails our done, clear everything out
				BufferQueue[SourceId].Empty();
				CurrentFrameValues[SourceId].Reset();
				NextFrameValues[SourceId].Reset();
				CurrentPCMBuffer[SourceId] = nullptr;
			}


			// If the source has HRTF processing enabled, run it through the spatializer
			if (bUseHRTFSpatializer[SourceId])
			{
				TSharedPtr<IAudioSpatialization> SpatializationPlugin = MixerDevice->SpatializationPluginInterface;

				AUDIO_MIXER_CHECK(SpatializationPlugin.IsValid());
				AUDIO_MIXER_CHECK(NumInputChans == 1);

				AudioPluginInputData.AudioBuffer = &Buffer;
				AudioPluginInputData.NumChannels = NumInputChans;
				AudioPluginInputData.SourceId = SourceId;
				AudioPluginInputData.SpatializationParams = &SpatParams[SourceId];

				AudioPluginOutputData[SourceId].AudioBuffer.Reset();
				AudioPluginOutputData[SourceId].AudioBuffer.AddZeroed(2 * NumFrames);

				SpatializationPlugin->ProcessAudio(AudioPluginInputData, AudioPluginOutputData[SourceId]);

				// We are now a 2-channel file and should not be spatialized using normal 3d spatialization
				NumPostEffectChannels[SourceId] = 2;

				PostEffectBuffers[SourceId] = &AudioPluginOutputData[SourceId].AudioBuffer;
			}
			else
			{
				// Otherwise our pre- and post-effect channels are the same as the input channels
				NumPostEffectChannels[SourceId] = NumInputChannels[SourceId];

				// Set the ptr to use for post-effect buffers
				PostEffectBuffers[SourceId] = &Buffer;
			}
		}
	}

	void FMixerSourceManager::ComputeOutputBuffersForIdRange(const int32 SourceIdStart, const int32 SourceIdEnd)
	{
		SCOPE_CYCLE_COUNTER(STAT_AudioMixerSourceOutputBuffers);

		const int32 NumFrames = MixerDevice->GetNumOutputFrames();
		const int32 NumOutputChannels = MixerDevice->GetNumDeviceChannels();

		// Reset the dry/wet buffers for all the sources
		const int32 NumOutputSamples = NumFrames * NumOutputChannels;

		for (int32 SourceId = SourceIdStart; SourceId < SourceIdEnd; ++SourceId)
		{
			OutputBuffer[SourceId].Reset();
			OutputBuffer[SourceId].AddZeroed(NumOutputSamples);
		}

		for (int32 SourceId = SourceIdStart; SourceId < SourceIdEnd; ++SourceId)
		{
			// Don't need to compute anything if the source is not playing or paused (it will remain at 0.0 volume)
			// Note that effect chains will still be able to continue to compute audio output. The source output 
			// will simply stop being read from.
			if (!bIsBusy[SourceId] || !bIsPlaying[SourceId] || bIsPaused[SourceId])
			{
				continue;
			}

			for (int32 Frame = 0; Frame < NumFrames; ++Frame)
			{
				const int32 PostEffectChannels = NumPostEffectChannels[SourceId];

#if ENABLE_AUDIO_OUTPUT_DEBUGGING
				FSineOsc& SineOsc = DebugOutputGenerators[SourceId];
				float SourceSampleValue = SineOsc();
#else
				float SourceSampleValue = 0.0f;
#endif

				// For each source channel, compute the output channel mapping
				for (int32 SourceChannel = 0; SourceChannel < PostEffectChannels; ++SourceChannel)
				{

#if !ENABLE_AUDIO_OUTPUT_DEBUGGING
					const int32 SourceSampleIndex = Frame * PostEffectChannels + SourceChannel;
					TArray<float>* Buffer = PostEffectBuffers[SourceId];
					SourceSampleValue = (*Buffer)[SourceSampleIndex];
#endif
					// Make sure that our channel map is appropriate for the source channel and output channel count!
					TArray<float>& Scratch = ScratchChannelMap[SourceId];
					ChannelMapParam[SourceId].GetChannelMap(Scratch);
					AUDIO_MIXER_CHECK(Scratch.Num() == PostEffectChannels * NumOutputChannels);

					for (int32 OutputChannel = 0; OutputChannel < NumOutputChannels; ++OutputChannel)
					{
						// Look up the channel map value (maps input channels to output channels) for the source
						// This is the step that either applies the spatialization algorithm or just maps a 2d sound
						const int32 ChannelMapIndex = NumOutputChannels * SourceChannel + OutputChannel;
						const float ChannelMapValue = Scratch[ChannelMapIndex];

						// If we have a non-zero sample value, write it out. Note that most 3d audio channel maps
						// for surround sound will result in 0.0 sample values so this branch should save a bunch of multiplies + adds
						if (ChannelMapValue > 0.0f)
						{
							AUDIO_MIXER_CHECK(ChannelMapValue >= 0.0f && ChannelMapValue <= 1.0f);

							// Scale the input source sample for this source channel value
							const float SampleValue = SourceSampleValue * ChannelMapValue;

							const int32 OutputSampleIndex = Frame * NumOutputChannels + OutputChannel;

							OutputBuffer[SourceId][OutputSampleIndex] += SampleValue;
						}
					}
				}
			}
		}
	}

	void FMixerSourceManager::MixOutputBuffers(const int32 SourceId, TArray<float>& OutWetBuffer, const float SendLevel) const
	{
		AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);
		AUDIO_MIXER_CHECK(SourceId < NumTotalSources);

		if (SendLevel > 0.0f)
		{
			for (int32 SampleIndex = 0; SampleIndex < OutWetBuffer.Num(); ++SampleIndex)
			{
				OutWetBuffer[SampleIndex] += OutputBuffer[SourceId][SampleIndex] * SendLevel;
			}
		}
	}

	void FMixerSourceManager::UpdateDeviceChannelCount(const int32 InNumOutputChannels)
	{
		// Update all source's to appropriate channel maps
		for (int32 SourceId = 0; SourceId < NumTotalSources; ++SourceId)
		{
			// Don't need to do anything if it's not active
			if (!bIsActive[SourceId])
			{
				continue;
			}

			ScratchChannelMap[SourceId].Reset();
			const int32 NumSoureChannels = bUseHRTFSpatializer[SourceId] ? 2 : NumInputChannels[SourceId];

			// If this is a 3d source, then just zero out the channel map, it'll cause a temporary blip
			// but it should reset in the next tick
			if (bIs3D[SourceId])
			{
				GameThreadInfo.bNeedsSpeakerMap[SourceId] = true;
				ScratchChannelMap[SourceId].AddZeroed(NumSoureChannels * InNumOutputChannels);
			}
			// If it's a 2D sound, then just get a new channel map appropriate for the new device channel count
			else
			{
				ScratchChannelMap[SourceId].Reset();
				MixerDevice->Get2DChannelMap(NumSoureChannels, InNumOutputChannels, bIsCenterChannelOnly[SourceId], ScratchChannelMap[SourceId]);
			}
			ChannelMapParam[SourceId].SetChannelMap(ScratchChannelMap[SourceId]);
		}
	}

	void FMixerSourceManager::UpdateSourceEffectChain(const uint32 InSourceEffectChainId, const TArray<FSourceEffectChainEntry>& InSourceEffectChain, const bool bPlayEffectChainTails)
	{
		AudioMixerThreadCommand([this, InSourceEffectChainId, InSourceEffectChain, bPlayEffectChainTails]()
		{
			FSoundEffectSourceInitData InitData;
			InitData.AudioClock = MixerDevice->GetAudioClock();
			InitData.SampleRate = AUDIO_SAMPLE_RATE;
			for (int32 SourceId = 0; SourceId < NumTotalSources; ++SourceId)
			{
				if (SourceEffectChainId[SourceId] == InSourceEffectChainId)
				{
					bEffectTailsDone[SourceId] = !bPlayEffectChainTails;

					// Check to see if the chain didn't actually change
					TArray<FSoundEffectSource *>& ThisSourceEffectChain = SourceEffects[SourceId];
					bool bReset = false;
					if (InSourceEffectChain.Num() == ThisSourceEffectChain.Num())
					{
						for (int32 SourceEffectId = 0; SourceEffectId < ThisSourceEffectChain.Num(); ++SourceEffectId)
						{
							const FSourceEffectChainEntry& ChainEntry = InSourceEffectChain[SourceEffectId];

							FSoundEffectSource* SourceEffectInstance = ThisSourceEffectChain[SourceEffectId];
							if (!SourceEffectInstance->IsParentPreset(ChainEntry.Preset))
							{
								// As soon as one of the effects change or is not the same, then we need to rebuild the effect graph
								bReset = true;
								break;
							}

							// Otherwise just update if it's just to bypass
							SourceEffectInstance->SetEnabled(!ChainEntry.bBypass);
						}
					}
					else
					{
						bReset = true;
					}

					if (bReset)
					{
						InitData.NumSourceChannels = NumInputChannels[SourceId];

						if (NumInputFrames[SourceId] != INDEX_NONE)
						{
							InitData.SourceDuration = (float)NumInputFrames[SourceId] / AUDIO_SAMPLE_RATE;
						}
						else
						{
							// Procedural sound waves have no known duration
							InitData.SourceDuration = (float)INDEX_NONE;
						}

						// First reset the source effect chain
						ResetSourceEffectChain(SourceId);

						// Rebuild it
						BuildSourceEffectChain(SourceId, InitData, InSourceEffectChain);
					}
				}
			}
		});
	}

	void FMixerSourceManager::ComputeNextBlockOfSamples()
	{
		AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

		SCOPE_CYCLE_COUNTER(STAT_AudioMixerSourceManagerUpdate);

		if (bPumpQueue)
		{
			bPumpQueue = false;
			PumpCommandQueue();
		}

		if (NumSourceWorkers > 0)
		{
			AUDIO_MIXER_CHECK(SourceWorkers.Num() == NumSourceWorkers);
			for (int32 i = 0; i < SourceWorkers.Num(); ++i)
			{
				SourceWorkers[i]->StartBackgroundTask();
			}

			for (int32 i = 0; i < SourceWorkers.Num(); ++i)
			{
				SourceWorkers[i]->EnsureCompletion();
			}
		}
		else
		{
			// Get the next block of frames from the source buffers
			ComputeSourceBuffersForIdRange(0, NumTotalSources);

			// Compute the audio source buffers after their individual effect chain processing
			ComputePostSourceEffectBufferForIdRange(0, NumTotalSources);

			// Get the audio for the output buffers
			ComputeOutputBuffersForIdRange(0, NumTotalSources);
		}

		// Update the game thread copy of source doneness
		for (int32 SourceId = 0; SourceId < NumTotalSources; ++SourceId)
		{		
			GameThreadInfo.bIsDone[SourceId] = bIsDone[SourceId];
			GameThreadInfo.bEffectTailsDone[SourceId] = bEffectTailsDone[SourceId];
		}
	}

	void FMixerSourceManager::AudioMixerThreadCommand(TFunction<void()> InFunction)
	{
		// Add the function to the command queue
		SourceCommandQueue.Enqueue(MoveTemp(InFunction));
	}

	void FMixerSourceManager::PumpCommandQueue()
	{
		AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

		// Pop and execute all the commands that came since last update tick
		TFunction<void()> CommandFunction;
		while (SourceCommandQueue.Dequeue(CommandFunction))
		{
			CommandFunction();
		}
	}
}
