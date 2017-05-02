// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AudioMixerSourceManager.h"
#include "AudioMixerSource.h"
#include "AudioMixerDevice.h"
#include "AudioMixerSourceVoice.h"
#include "AudioMixerSubmix.h"
#include "IAudioExtensionPlugin.h"

DEFINE_STAT(STAT_AudioMixerHRTF);

static int32 DisableParallelSourceProcessingCvar = 0;
FAutoConsoleVariableRef CVarDisableParallelSourceProcessing(
	TEXT("au.DisableParallelSourceProcessing"),
	DisableParallelSourceProcessingCvar,
	TEXT("Disables using async tasks for processing sources.\n")
	TEXT("0: Not Disabled, 1: Disabled"),
	ECVF_Default);


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
		, NumInterpFramesInverse(0.0f)
		, Frame(0.0f)
		, bIsInit(true)
		, bIsDone(false)
	{
		NumInterpFramesInverse = (NumInterpFrames == 0.0f ? 0.0f : (1.0f / NumInterpFrames));
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
			bIsDone = true;
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
		if (!bIsDone)
		{
			float Alpha = Frame * NumInterpFramesInverse;
			if (Alpha >= 1.0f)
			{
				bIsDone = true;
				CurrentValue = EndValue;
			}
			else
			{
				CurrentValue = FMath::Lerp(StartValue, EndValue, Alpha);
			}

			Frame += 1.0f;
		}

		return CurrentValue;
	}

	float FSourceParam::GetValue() const
	{
		return CurrentValue;
	}

	/*************************************************************************
	* FMixerSourceManager::FSourceInfo
	**************************************************************************/

	FMixerSourceManager::FSourceInfo::FSourceInfo()
		: PitchSourceParam(FSourceParam(128))
		, VolumeSourceParam(FSourceParam(256))
		, LPFCutoffFrequencyParam(FSourceParam(128))
		, ChannelMapParam(FSourceChannelMap(256))
	{
		// Note: initialization is all done by FMixerSourceManager::Init
	}

	FMixerSourceManager::FSourceInfo::~FSourceInfo()
	{
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

		MixerSources.Init(nullptr, NumTotalSources);

		SourceInfos.AddDefaulted(NumTotalSources);
		for (int32 i = 0; i < NumTotalSources; ++i)
		{
			FSourceInfo& SourceInfo = SourceInfos[i];

			//SourceInfo.BufferQueue
			SourceInfo.BufferQueueListener = nullptr;
			SourceInfo.CurrentPCMBuffer = nullptr;	
			SourceInfo.CurrentAudioChunkNumFrames = 0;
			//SourceInfo.SourceBuffer
			//SourceInfo.AudioPluginOutputData
			//SourceInfo.CurrentFrameValues
			//SourceINfo.NextFrameValues
			SourceInfo.CurrentFrameAlpha = 0.0f;
			SourceInfo.CurrentFrameIndex = 0;
			SourceInfo.NumFramesPlayed = 0;

			// These are in the constructor
			//SourceInfo.PitchSourceParam = FSourceParam(128);
			//SourceInfo.VolumeSourceParam = FSourceParam(256);
			//SourceInfo.LPFCutoffFrequencyParam = FSourceParam(128);
			//SourceInfo.ChannelMapParam = FSourceChannelMap(256);
		
			//SourceInfo.SpatParams
			//SourceInfo.ScratchChannelMap
			//SourceInfo.LowPassFilters

			SourceInfo.SourceEffectChainId = INDEX_NONE;
			//SourceInfo.SourceEffects
			//SourceInfo.SourceEffectPresets

			SourceInfo.SourceEnvelopeFollower = Audio::FEnvelopeFollower(AUDIO_SAMPLE_RATE, 10, 100, Audio::EPeakMode::Peak);
			SourceInfo.SourceEnvelopeValue = 0.0f;
			SourceInfo.bEffectTailsDone = false;
			//SourceInfo.SourceEffectInputData
			//SourceInfo.SourceEffectOutputData
			//SourceInfo.ReverbPluginOutputBuffer
			SourceInfo.PostEffectBuffers = nullptr;
			//SourceInfo.OutputBuffer
		
			SourceInfo.bIs3D = false;
			SourceInfo.bIsCenterChannelOnly = false;
			SourceInfo.bIsActive = false;
			SourceInfo.bIsPlaying = false;
			SourceInfo.bIsPaused = false;
			SourceInfo.bIsDone = false;
			SourceInfo.bIsBusy = false;
			SourceInfo.bUseHRTFSpatializer = false;
			SourceInfo.bUseOcclusionPlugin = false;
			SourceInfo.bUseReverbPlugin = false;
			SourceInfo.bHasStarted = false;
			SourceInfo.bIsDebugMode = false;
			//SourceInfo.DebugName

			SourceInfo.NumInputChannels = 0;
			SourceInfo.NumPostEffectChannels = 0;
			SourceInfo.NumInputFrames = 0;
		}
		
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
			FSourceInfo& SourceInfo = SourceInfos[SourceId];

			SourceInfo.SourceBuffer.Reset(NumFrames * 8);
			SourceInfo.AudioPluginOutputData.AudioBuffer.Reset(NumFrames * 2);
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

		FSourceInfo& SourceInfo = SourceInfos[SourceId];

#if AUDIO_MIXER_ENABLE_DEBUG_MODE
		if (SourceInfo.bIsDebugMode)
		{
			DebugSoloSources.Remove(SourceId);
		}
#endif

		// Call OnRelease on the BufferQueueListener to give it a chance 
		// to release any resources it owns on the audio render thread
		if (SourceInfo.BufferQueueListener)
		{
			SourceInfo.BufferQueueListener->OnRelease();
			SourceInfo.BufferQueueListener = nullptr;
		}

		// Remove the mixer source from its submix sends
		TMap<uint32, FMixerSourceSubmixSend>& SubmixSends = MixerSources[SourceId]->GetSubmixSends();
		for (auto SubmixSendItem : SubmixSends)
		{
			SubmixSendItem.Value.Submix->RemoveSourceVoice(MixerSources[SourceId]);
		}

		// Notify plugin effects
		if (SourceInfo.bUseHRTFSpatializer)
		{
			MixerDevice->SpatializationPluginInterface->OnReleaseSource(SourceId);
		}

		if (SourceInfo.bUseOcclusionPlugin)
		{
			MixerDevice->OcclusionInterface->OnReleaseSource(SourceId);
		}

		if (SourceInfo.bUseReverbPlugin)
		{
			MixerDevice->ReverbPluginInterface->OnReleaseSource(SourceId);
		}

		// Delete the source effects
		SourceInfo.SourceEffectChainId = INDEX_NONE;
		ResetSourceEffectChain(SourceId);

		SourceInfo.SourceEnvelopeFollower.Reset();
		SourceInfo.bEffectTailsDone = true;

		// Delete the mixer source and null the slot
		delete MixerSources[SourceId];
		MixerSources[SourceId] = nullptr;

		// Reset all state and data
		SourceInfo.PitchSourceParam.Reset();
		SourceInfo.VolumeSourceParam.Reset();
		SourceInfo.LPFCutoffFrequencyParam.Reset();

		SourceInfo.LowPassFilters.Reset();
		SourceInfo.ChannelMapParam.Reset();
		SourceInfo.BufferQueue.Empty();
		SourceInfo.CurrentPCMBuffer = nullptr;
		SourceInfo.CurrentAudioChunkNumFrames = 0;
		SourceInfo.SourceBuffer.Reset();
		SourceInfo.AudioPluginOutputData.AudioBuffer.Reset();
		SourceInfo.CurrentFrameValues.Reset();
		SourceInfo.NextFrameValues.Reset();
		SourceInfo.CurrentFrameAlpha = 0.0f;
		SourceInfo.CurrentFrameIndex = 0;
		SourceInfo.NumFramesPlayed = 0;
		SourceInfo.PostEffectBuffers = nullptr;
		SourceInfo.OutputBuffer.Reset();
		SourceInfo.bIs3D = false;
		SourceInfo.bIsCenterChannelOnly = false;
		SourceInfo.bIsActive = false;
		SourceInfo.bIsPlaying = false;
		SourceInfo.bIsDone = true;
		SourceInfo.bIsPaused = false;
		SourceInfo.bIsBusy = false;
		SourceInfo.bUseHRTFSpatializer = false;
		SourceInfo.bUseOcclusionPlugin = false;
		SourceInfo.bUseReverbPlugin = false;
		SourceInfo.bHasStarted = false;
		SourceInfo.bIsDebugMode = false;
		SourceInfo.DebugName = FString();
		SourceInfo.NumInputChannels = 0;
		SourceInfo.NumPostEffectChannels = 0;

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
			FSourceInfo& SourceInfo = SourceInfos[SourceId];
			SourceInfo.SourceEffects.Add(NewEffect);

			// Add a slot entry for the preset so it can change while running. This will get sent to the running effect instance if the preset changes.
			SourceInfo.SourceEffectPresets.Add(nullptr);
		}
	}

	void FMixerSourceManager::ResetSourceEffectChain(const int32 SourceId)
	{
		FSourceInfo& SourceInfo = SourceInfos[SourceId];

		for (int32 i = 0; i < SourceInfo.SourceEffects.Num(); ++i)
		{
			SourceInfo.SourceEffects[i]->UnregisterWithPreset();
			delete SourceInfo.SourceEffects[i];
			SourceInfo.SourceEffects[i] = nullptr;
		}
		SourceInfo.SourceEffects.Reset();

		for (int32 i = 0; i < SourceInfo.SourceEffectPresets.Num(); ++i)
		{
			SourceInfo.SourceEffectPresets[i] = nullptr;
		}
		SourceInfo.SourceEffectPresets.Reset();
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

			FSourceInfo& SourceInfo = SourceInfos[SourceId];

			SourceInfo.bIsPlaying = false;
			SourceInfo.bIsPaused = false;
			SourceInfo.bIsActive = true;
			SourceInfo.bIsBusy = true;
			SourceInfo.bIsDone = false;
			SourceInfo.bUseHRTFSpatializer = InitParams.bUseHRTFSpatialization;

			SourceInfo.BufferQueueListener = InitParams.BufferQueueListener;
			SourceInfo.NumInputChannels = InitParams.NumInputChannels;
			SourceInfo.NumInputFrames = InitParams.NumInputFrames;

			// Initialize the number of per-source LPF filters based on input channels
			SourceInfo.LowPassFilters.AddDefaulted(InitParams.NumInputChannels);

			// Create the spatialization plugin source effect
			if (InitParams.bUseHRTFSpatialization)
			{
				MixerDevice->SpatializationPluginInterface->OnInitSource(SourceId, InitParams.AudioComponentUserID, 
					InitParams.SpatializationPluginSettings);
			}

			// Create the occlusion plugin source effect
			if (InitParams.OcclusionPluginSettings != nullptr)
			{
				MixerDevice->OcclusionInterface->OnInitSource(SourceId, InitParams.AudioComponentUserID, InitParams.OcclusionPluginSettings);
				SourceInfo.bUseOcclusionPlugin = true;
			}

			// Create the reverb plugin source effect
			if (InitParams.ReverbPluginSettings != nullptr)
			{
				MixerDevice->ReverbPluginInterface->OnInitSource(SourceId, InitParams.AudioComponentUserID, InitParams.ReverbPluginSettings);
				SourceInfo.bUseReverbPlugin = true;
			}

			// Default all sounds to not consider effect chain tails when playing
			SourceInfo.bEffectTailsDone = true;

			// Copy the source effect chain if the channel count is 1 or 2
			if (InitParams.NumInputChannels <= 2)
			{
				// If we're told to care about effect chain tails, then we're not allowed
				// to stop playing until the effect chain tails are finished
				SourceInfo.bEffectTailsDone = !InitParams.bPlayEffectChainTails;

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

				SourceInfo.SourceEffectChainId = InitParams.SourceEffectChainId;
				BuildSourceEffectChain(SourceId, InitData, InitParams.SourceEffectChain);
			}

			SourceInfo.CurrentFrameValues.Init(0.0f, InitParams.NumInputChannels);
			SourceInfo.NextFrameValues.Init(0.0f, InitParams.NumInputChannels);

			AUDIO_MIXER_CHECK(MixerSources[SourceId] == nullptr);
			MixerSources[SourceId] = InitParams.SourceVoice;

			// Loop through the source's sends and add this source to those submixes with the send info
			for (int32 i = 0; i < InitParams.SubmixSends.Num(); ++i)
			{
				const FMixerSourceSubmixSend& MixerSourceSend = InitParams.SubmixSends[i];
				MixerSourceSend.Submix->AddOrSetSourceVoice(InitParams.SourceVoice, MixerSourceSend.SendLevel);
			}

#if AUDIO_MIXER_ENABLE_DEBUG_MODE
			AUDIO_MIXER_CHECK(!SourceInfo.bIsDebugMode);
			SourceInfo.bIsDebugMode = InitParams.bIsDebugMode;

			AUDIO_MIXER_CHECK(SourceInfo.DebugName.IsEmpty());
			SourceInfo.DebugName = InitParams.DebugName;
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

			FSourceInfo& SourceInfo = SourceInfos[SourceId];

			SourceInfo.bIsPlaying = true;
			SourceInfo.bIsPaused = false;
			SourceInfo.bIsActive = true;

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

			FSourceInfo& SourceInfo = SourceInfos[SourceId];

			SourceInfo.bIsPlaying = false;
			SourceInfo.bIsPaused = false;
			SourceInfo.bIsActive = false;

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

			FSourceInfo& SourceInfo = SourceInfos[SourceId];

			SourceInfo.bIsPaused = true;
			SourceInfo.bIsActive = false;
		});
	}

	void FMixerSourceManager::SetPitch(const int32 SourceId, const float Pitch)
	{
		AUDIO_MIXER_CHECK(SourceId < NumTotalSources);
		AUDIO_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);

		AudioMixerThreadCommand([this, SourceId, Pitch]()
		{
			AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

			SourceInfos[SourceId].PitchSourceParam.SetValue(Pitch);
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

			SourceInfos[SourceId].VolumeSourceParam.SetValue(Volume);
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

			SourceInfos[SourceId].SpatParams = InParams;
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

			FSourceInfo& SourceInfo = SourceInfos[SourceId];

			// Set whether or not this is a 3d channel map and if its center channel only. Used for reseting channel maps on device change.
			SourceInfo.bIs3D = bInIs3D;
			SourceInfo.bIsCenterChannelOnly = bInIsCenterChannelOnly;

			// Fix up the channel map in case device output count changed
			const int32 NumSourceChannels = SourceInfo.bUseHRTFSpatializer ? 2 : SourceInfo.NumInputChannels;
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
				SourceInfo.ChannelMapParam.SetChannelMap(NewChannelMap);
			}
			else
			{
				GameThreadInfo.bNeedsSpeakerMap[SourceId] = false;
				SourceInfo.ChannelMapParam.SetChannelMap(ChannelMap);
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
			SourceInfos[SourceId].LPFCutoffFrequencyParam.SetValue(NormalizedFrequency);
		});
	}

	void FMixerSourceManager::SubmitBuffer(const int32 SourceId, FMixerSourceBufferPtr InSourceVoiceBuffer, const bool bSubmitSynchronously)
	{
		AUDIO_MIXER_CHECK(SourceId < NumTotalSources);
		AUDIO_MIXER_CHECK(InSourceVoiceBuffer->AudioBytes <= (uint32)InSourceVoiceBuffer->AudioData.Num());

		// If we're submitting synchronously then don't use AudioMixerThreadCommand but submit directly
		if (bSubmitSynchronously)
		{
			SourceInfos[SourceId].BufferQueue.Enqueue(InSourceVoiceBuffer);
		}
		else
		{
			// make sure we're on the game/audio thread
			AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

			// Use a thread command
			AudioMixerThreadCommand([this, SourceId, InSourceVoiceBuffer]()
			{
				AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

				SourceInfos[SourceId].BufferQueue.Enqueue(InSourceVoiceBuffer);
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
		return SourceInfos[SourceId].NumFramesPlayed;
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
		FSourceInfo& SourceInfo = SourceInfos[SourceId];

		const int32 NumChannels = SourceInfo.NumInputChannels;

		// Check if the next frame index is out of range of the total number of frames we have in our current audio buffer
		bool bNextFrameOutOfRange = (SourceInfo.CurrentFrameIndex + 1) >= SourceInfo.CurrentAudioChunkNumFrames;
		bool bCurrentFrameOutOfRange = SourceInfo.CurrentFrameIndex >= SourceInfo.CurrentAudioChunkNumFrames;

		bool bReadCurrentFrame = true;

		// Check the boolean conditions that determine if we need to pop buffers from our queue (in PCMRT case) *OR* loop back (looping PCM data)
		while (bNextFrameOutOfRange || bCurrentFrameOutOfRange)
		{
			// If our current frame is in range, but next frame isn't, read the current frame now to avoid pops when transitioning between buffers
			if (bNextFrameOutOfRange && !bCurrentFrameOutOfRange)
			{
				// Don't need to read the current frame audio after reading new audio chunk
				bReadCurrentFrame = false;

				AUDIO_MIXER_CHECK(SourceInfo.CurrentPCMBuffer.IsValid());
				const int16* AudioData = (const int16*)SourceInfo.CurrentPCMBuffer->AudioData.GetData();
				const int32 CurrentSampleIndex = SourceInfo.CurrentFrameIndex * NumChannels;

				for (int32 Channel = 0; Channel < NumChannels; ++Channel)
				{
					SourceInfo.CurrentFrameValues[Channel] = (float)AudioData[CurrentSampleIndex + Channel] * ONEOVERSHORTMAX;
				}
			}

			// If this is our first PCM buffer, we don't need to do a callback to get more audio
			if (SourceInfo.CurrentPCMBuffer.IsValid())
			{
				if (SourceInfo.CurrentPCMBuffer->LoopCount == Audio::LOOP_FOREVER && !SourceInfo.CurrentPCMBuffer->bRealTimeBuffer)
				{
					AUDIO_MIXER_DEBUG_LOG(SourceId, TEXT("Hit Loop boundary, looping."));

					SourceInfo.CurrentFrameIndex = FMath::Max(SourceInfo.CurrentFrameIndex - SourceInfo.CurrentAudioChunkNumFrames, 0);
					break;
				}

				SourceInfo.BufferQueueListener->OnSourceBufferEnd();
			}

			// If we have audio in our queue, we're still playing
			if (!SourceInfo.BufferQueue.IsEmpty())
			{
				FMixerSourceBufferPtr NewBufferPtr;
				SourceInfo.BufferQueue.Dequeue(NewBufferPtr);
				SourceInfo.CurrentPCMBuffer = NewBufferPtr;

				AUDIO_MIXER_CHECK(MixerSources[SourceId]->NumBuffersQueued.GetValue() > 0);

				MixerSources[SourceId]->NumBuffersQueued.Decrement();

				SourceInfo.CurrentAudioChunkNumFrames = SourceInfo.CurrentPCMBuffer->AudioBytes / (NUM_BYTES_PER_SAMPLE * NumChannels);

				// Subtract the number of frames in the current buffer from our frame index.
				// Note: if this is the first time we're playing, CurrentFrameIndex will be 0
				if (bReadCurrentFrame)
				{
					SourceInfo.CurrentFrameIndex = FMath::Max(SourceInfo.CurrentFrameIndex - SourceInfo.CurrentAudioChunkNumFrames, 0);
				}
				else
				{
					// Since we're not reading the current frame, we allow the current frame index to be negative (NextFrameIndex will then be 0)
					// This prevents dropping a frame of audio on the buffer boundary
					SourceInfo.CurrentFrameIndex = -1;
				}
			}
			else
			{
				SourceInfo.bIsDone = true;
				return;
			}

			bNextFrameOutOfRange = (SourceInfo.CurrentFrameIndex + 1) >= SourceInfo.CurrentAudioChunkNumFrames;
			bCurrentFrameOutOfRange = SourceInfo.CurrentFrameIndex >= SourceInfo.CurrentAudioChunkNumFrames;
		}

		if (SourceInfo.CurrentPCMBuffer.IsValid())
		{
			// Grab the 16-bit PCM audio data (which could be a new audio chunk from previous ReadSourceFrame call)
			const int16* AudioData = (const int16*)SourceInfo.CurrentPCMBuffer->AudioData.GetData();
			const int32 NextSampleIndex = (SourceInfo.CurrentFrameIndex + 1)  * NumChannels;

			if (bReadCurrentFrame)
			{
				const int32 CurrentSampleIndex = SourceInfo.CurrentFrameIndex * NumChannels;
				for (int32 Channel = 0; Channel < NumChannels; ++Channel)
				{
					SourceInfo.CurrentFrameValues[Channel] = (float)AudioData[CurrentSampleIndex + Channel] * ONEOVERSHORTMAX;
					SourceInfo.NextFrameValues[Channel] = (float)AudioData[NextSampleIndex + Channel] * ONEOVERSHORTMAX;
				}
			}
			else
			{
				for (int32 Channel = 0; Channel < NumChannels; ++Channel)
				{
					SourceInfo.NextFrameValues[Channel] = (float)AudioData[NextSampleIndex + Channel] * ONEOVERSHORTMAX;
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
			FSourceInfo& SourceInfo = SourceInfos[SourceId];

			SourceInfo.SourceBuffer.Reset();

			if (!SourceInfo.bIsBusy || !SourceInfo.bIsPlaying || SourceInfo.bIsPaused)
			{
				continue;
			}

			// Fill array with elements all at once to avoid sequential Add() operation overhead.
			const int32 NumSourceChannels = SourceInfo.NumInputChannels;
			SourceInfo.SourceBuffer.AddUninitialized(NumFrames * NumSourceChannels);
			int32 SampleIndex = 0;

			for (int32 Frame = 0; Frame < NumFrames; ++Frame)
			{
				// If the source is done, then we'll just write out 0s
				if (!SourceInfo.bIsDone)
				{
					// Whether or not we need to read another sample from the source buffers
					// If we haven't yet played any frames, then we will need to read the first source samples no matter what
					bool bReadNextSample = !SourceInfo.bHasStarted;

					// Reset that we've started generating audio
					SourceInfo.bHasStarted = true;

					// Update the PrevFrameIndex value for the source based on alpha value
					while (SourceInfo.CurrentFrameAlpha >= 1.0f)
					{
						// Our inter-frame alpha lerping value is causing us to read new source frames
						bReadNextSample = true;

						// Bump up the current frame index
						SourceInfo.CurrentFrameIndex++;

						// Bump up the frames played -- this is tracking the total frames in source file played
						// CurrentFrameIndex can wrap for looping sounds so won't be accurate in that case
						SourceInfo.NumFramesPlayed++;

						SourceInfo.CurrentFrameAlpha -= 1.0f;
					}

					// If our alpha parameter caused us to jump to a new source frame, we need
					// read new samples into our prev and next frame sample data
					if (bReadNextSample)
					{
						ReadSourceFrame(SourceId);
					}
				}

				// If we've finished reading all buffer data, then just write out 0s
				if (SourceInfo.bIsDone)
				{
					for (int32 Channel = 0; Channel < NumSourceChannels; ++Channel)
					{
						SourceInfo.SourceBuffer[SampleIndex++] = 0.0f;
					}
				}
				else
				{
					// Get the volume value of the source at this frame index
					for (int32 Channel = 0; Channel < NumSourceChannels; ++Channel)
					{
						const float CurrFrameValue = SourceInfo.CurrentFrameValues[Channel];
						const float NextFrameValue = SourceInfo.NextFrameValues[Channel];
						const float CurrentAlpha = SourceInfo.CurrentFrameAlpha;

						SampleValue = FMath::Lerp(CurrFrameValue, NextFrameValue, CurrentAlpha);
						SourceInfo.SourceBuffer[SampleIndex++] = SampleValue;
					}
					const float CurrentPitchScale = SourceInfo.PitchSourceParam.Update();

					SourceInfo.CurrentFrameAlpha += CurrentPitchScale;
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
			FSourceInfo& SourceInfo = SourceInfos[SourceId];

			if (!SourceInfo.bIsBusy || !SourceInfo.bIsPlaying || SourceInfo.bIsPaused)
			{
				continue;
			}

			// Get the source buffer
			TArray<float>& Buffer = SourceInfo.SourceBuffer;

			const int32 NumInputChans = SourceInfo.NumInputChannels;

			float CurrentVolumeValue = SourceInfo.VolumeSourceParam.GetValue();

			// Process the per-source LPF if the source hasn't actually been finished
			if (!SourceInfo.bIsDone)
			{
				for (int32 Frame = 0; Frame < NumFrames; ++Frame)
				{
					FSourceParam& LPFFrequencyParam = SourceInfo.LPFCutoffFrequencyParam;

					// Update the frequency at the block rate
					const float LPFFreq = LPFFrequencyParam.Update();

					// Update the volume
#if AUDIO_MIXER_ENABLE_DEBUG_MODE				
					CurrentVolumeValue = (bIsDebugModeEnabled && !SourceInfo.bIsDebugMode) ? 0.0f : SourceInfo.VolumeSourceParam.Update();

#else
					CurrentVolumeValue = SourceInfo.VolumeSourceParam.Update();
#endif

					for (int32 Channel = 0; Channel < NumInputChans; ++Channel)
					{
						SourceInfo.LowPassFilters[Channel].SetFrequency(LPFFreq);

						// Process the source through the filter
						const int32 SampleIndex = NumInputChans * Frame + Channel;
						Buffer[SampleIndex] = CurrentVolumeValue * SourceInfo.LowPassFilters[Channel].ProcessAudio(Buffer[SampleIndex]);
					}
				}
			}

			const int32 NumSamples = Buffer.Num();

			// Now process the effect chain if it exists
			if (SourceInfo.SourceEffects.Num() > 0 && (!SourceInfo.bIsDone || !SourceInfo.bEffectTailsDone))
			{
				SourceInfo.SourceEffectInputData.CurrentVolume = CurrentVolumeValue;

				SourceInfo.SourceEffectOutputData.AudioFrame.Reset();
				SourceInfo.SourceEffectOutputData.AudioFrame.AddZeroed(NumInputChans);

				SourceInfo.SourceEffectInputData.AudioFrame.Reset();
				SourceInfo.SourceEffectInputData.AudioFrame.AddZeroed(NumInputChans);

				// Process the effect chain for this buffer per frame
				for (int32 Sample = 0; Sample < NumSamples; Sample += NumInputChans)
				{
					// Get the buffer input sample
					for (int32 Chan = 0; Chan < NumInputChans; ++Chan)
					{
						SourceInfo.SourceEffectInputData.AudioFrame[Chan] = Buffer[Sample + Chan];
					}

					for (int32 SourceEffectIndex = 0; SourceEffectIndex < SourceInfo.SourceEffects.Num(); ++SourceEffectIndex)
					{
						if (SourceInfo.SourceEffects[SourceEffectIndex]->IsActive())
						{
							SourceInfo.SourceEffects[SourceEffectIndex]->Update();

							SourceInfo.SourceEffects[SourceEffectIndex]->ProcessAudio(SourceInfo.SourceEffectInputData, SourceInfo.SourceEffectOutputData);

							// Copy the output of the effect into the input so the next effect will get the outputs audio
							for (int32 Chan = 0; Chan < NumInputChans; ++Chan)
							{
								SourceInfo.SourceEffectInputData.AudioFrame[Chan] = SourceInfo.SourceEffectOutputData.AudioFrame[Chan];
							}
						}
					}

					// Copy audio frame back to the buffer
					for (int32 Chan = 0; Chan < NumInputChans; ++Chan)
					{
						Buffer[Sample + Chan] = SourceInfo.SourceEffectInputData.AudioFrame[Chan];
					}
				}
			}

			bool bReverbPluginUsed = false;
			if (SourceInfo.bUseReverbPlugin)
			{
				const FSpatializationParams* SourceSpatParams = &SourceInfo.SpatParams;

				// Move the audio buffer to the reverb plugin buffer
				AudioPluginInputData.SourceId = SourceId;
				AudioPluginInputData.AudioBuffer = &Buffer;
				AudioPluginInputData.SpatializationParams = SourceSpatParams;
				AudioPluginInputData.NumChannels = NumInputChans;
				SourceInfo.AudioPluginOutputData.AudioBuffer.Reset();
				SourceInfo.AudioPluginOutputData.AudioBuffer.AddZeroed(AudioPluginInputData.AudioBuffer->Num());

				MixerDevice->ReverbPluginInterface->ProcessSourceAudio(AudioPluginInputData, SourceInfo.AudioPluginOutputData);

				// Make sure the buffer counts didn't change and are still the same size
				AUDIO_MIXER_CHECK(SourceInfo.AudioPluginOutputData.AudioBuffer.Num() == Buffer.Num());

				// Copy the reverb-processed data back to the source buffer
				SourceInfo.ReverbPluginOutputBuffer.Reset();
				SourceInfo.ReverbPluginOutputBuffer.Append(SourceInfo.AudioPluginOutputData.AudioBuffer);

				bReverbPluginUsed = true;
			}

			if (SourceInfo.bUseOcclusionPlugin)
			{
				const FSpatializationParams* SourceSpatParams = &SourceInfo.SpatParams;

				// Move the audio buffer to the occlusion plugin buffer
				AudioPluginInputData.SourceId = SourceId;
				AudioPluginInputData.AudioBuffer = &Buffer;
				AudioPluginInputData.SpatializationParams = SourceSpatParams;
				AudioPluginInputData.NumChannels = NumInputChans;

				SourceInfo.AudioPluginOutputData.AudioBuffer.Reset();
				SourceInfo.AudioPluginOutputData.AudioBuffer.AddZeroed(AudioPluginInputData.AudioBuffer->Num());

				MixerDevice->OcclusionInterface->ProcessAudio(AudioPluginInputData, SourceInfo.AudioPluginOutputData);

				// Make sure the buffer counts didn't change and are still the same size
				AUDIO_MIXER_CHECK(SourceInfo.AudioPluginOutputData.AudioBuffer.Num() == Buffer.Num());

				// Copy the occlusion-processed data back to the source buffer and mix with the reverb plugin output buffer
				if (bReverbPluginUsed)
				{
					for (int32 i = 0; i < Buffer.Num(); ++i)
					{
						Buffer[i] = SourceInfo.ReverbPluginOutputBuffer[i] + SourceInfo.AudioPluginOutputData.AudioBuffer[i];
					}
				}
				else
				{
					FMemory::Memcpy(Buffer.GetData(), SourceInfo.AudioPluginOutputData.AudioBuffer.GetData(), sizeof(float) * Buffer.Num());
				}
			}
			else if (bReverbPluginUsed)
			{
				for (int32 i = 0; i < Buffer.Num(); ++i)
				{
					Buffer[i] += SourceInfo.ReverbPluginOutputBuffer[i];
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
				SourceInfo.SourceEnvelopeFollower.ProcessAudio(AverageSampleValue);
			}

			// Copy the current value of the envelope follower (block-rate value)
			SourceInfo.SourceEnvelopeValue = SourceInfo.SourceEnvelopeFollower.GetCurrentValue();

			SourceInfo.bEffectTailsDone = SourceInfo.bEffectTailsDone || SourceInfo.SourceEnvelopeValue < ENVELOPE_TAIL_THRESHOLD;

			// Check the source effect tails condition
			if (SourceInfo.bIsDone && SourceInfo.bEffectTailsDone)
			{
				// If we're done and our tails our done, clear everything out
				SourceInfo.BufferQueue.Empty();
				SourceInfo.CurrentFrameValues.Reset();
				SourceInfo.NextFrameValues.Reset();
				SourceInfo.CurrentPCMBuffer = nullptr;
			}


			// If the source has HRTF processing enabled, run it through the spatializer
			if (SourceInfo.bUseHRTFSpatializer)
			{
				SCOPE_CYCLE_COUNTER(STAT_AudioMixerHRTF);

				TSharedPtr<IAudioSpatialization> SpatializationPlugin = MixerDevice->SpatializationPluginInterface;

				AUDIO_MIXER_CHECK(SpatializationPlugin.IsValid());
				AUDIO_MIXER_CHECK(NumInputChans == 1);

				AudioPluginInputData.AudioBuffer = &Buffer;
				AudioPluginInputData.NumChannels = NumInputChans;
				AudioPluginInputData.SourceId = SourceId;
				AudioPluginInputData.SpatializationParams = &SourceInfo.SpatParams;

				SourceInfo.AudioPluginOutputData.AudioBuffer.Reset();
				SourceInfo.AudioPluginOutputData.AudioBuffer.AddZeroed(2 * NumFrames);

				SpatializationPlugin->ProcessAudio(AudioPluginInputData, SourceInfo.AudioPluginOutputData);

				// We are now a 2-channel file and should not be spatialized using normal 3d spatialization
				SourceInfo.NumPostEffectChannels = 2;

				SourceInfo.PostEffectBuffers = &SourceInfo.AudioPluginOutputData.AudioBuffer;
			}
			else
			{
				// Otherwise our pre- and post-effect channels are the same as the input channels
				SourceInfo.NumPostEffectChannels = SourceInfo.NumInputChannels;

				// Set the ptr to use for post-effect buffers
				SourceInfo.PostEffectBuffers = &Buffer;
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
			FSourceInfo& SourceInfo = SourceInfos[SourceId];

			SourceInfo.OutputBuffer.Reset();
			SourceInfo.OutputBuffer.AddZeroed(NumOutputSamples);
		}

		for (int32 SourceId = SourceIdStart; SourceId < SourceIdEnd; ++SourceId)
		{
			FSourceInfo& SourceInfo = SourceInfos[SourceId];

			// Don't need to compute anything if the source is not playing or paused (it will remain at 0.0 volume)
			// Note that effect chains will still be able to continue to compute audio output. The source output 
			// will simply stop being read from.
			if (!SourceInfo.bIsBusy || !SourceInfo.bIsPlaying || SourceInfo.bIsPaused)
			{
				continue;
			}

			for (int32 Frame = 0; Frame < NumFrames; ++Frame)
			{
				const int32 PostEffectChannels = SourceInfo.NumPostEffectChannels;
				AUDIO_MIXER_CHECK(SourceInfo.ChannelMapParam.ChannelValues.Num() == PostEffectChannels * NumOutputChannels);

				float SourceSampleValue = 0.0f;

				// For each source channel, compute the output channel mapping
				for (int32 SourceChannel = 0; SourceChannel < PostEffectChannels; ++SourceChannel)
				{
					const int32 SourceSampleIndex = Frame * PostEffectChannels + SourceChannel;
					TArray<float>* Buffer = SourceInfo.PostEffectBuffers;
					SourceSampleValue = (*Buffer)[SourceSampleIndex];

					// Make sure that our channel map is appropriate for the source channel and output channel count!
					SourceInfo.ChannelMapParam.UpdateChannelMap();

					for (int32 OutputChannel = 0; OutputChannel < NumOutputChannels; ++OutputChannel)
					{
						// Look up the channel map value (maps input channels to output channels) for the source
						// This is the step that either applies the spatialization algorithm or just maps a 2d sound
						const int32 ChannelMapIndex = NumOutputChannels * SourceChannel + OutputChannel;
						const float ChannelMapValue = SourceInfo.ChannelMapParam.GetChannelValue(ChannelMapIndex);

						// If we have a non-zero sample value, write it out. Note that most 3d audio channel maps
						// for surround sound will result in 0.0 sample values so this branch should save a bunch of multiplies + adds
						if (ChannelMapValue > 0.0f)
						{
							AUDIO_MIXER_CHECK(ChannelMapValue >= 0.0f && ChannelMapValue <= 1.0f);

							// Scale the input source sample for this source channel value
							const float SampleValue = SourceSampleValue * ChannelMapValue;

							const int32 OutputSampleIndex = Frame * NumOutputChannels + OutputChannel;

							SourceInfo.OutputBuffer[OutputSampleIndex] += SampleValue;
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
			const FSourceInfo& SourceInfo = SourceInfos[SourceId];
			const TArray<float>& SourceOutputBuffer = SourceInfo.OutputBuffer;

			for (int32 SampleIndex = 0; SampleIndex < OutWetBuffer.Num(); ++SampleIndex)
			{
				OutWetBuffer[SampleIndex] += SourceOutputBuffer[SampleIndex] * SendLevel;
			}
		}
	}

	void FMixerSourceManager::UpdateDeviceChannelCount(const int32 InNumOutputChannels)
	{
		// Update all source's to appropriate channel maps
		for (int32 SourceId = 0; SourceId < NumTotalSources; ++SourceId)
		{
			FSourceInfo& SourceInfo = SourceInfos[SourceId];

			// Don't need to do anything if it's not active
			if (!SourceInfo.bIsActive)
			{
				continue;
			}

			SourceInfo.ScratchChannelMap.Reset();
			const int32 NumSoureChannels = SourceInfo.bUseHRTFSpatializer ? 2 : SourceInfo.NumInputChannels;

			// If this is a 3d source, then just zero out the channel map, it'll cause a temporary blip
			// but it should reset in the next tick
			if (SourceInfo.bIs3D)
			{
				GameThreadInfo.bNeedsSpeakerMap[SourceId] = true;
				SourceInfo.ScratchChannelMap.AddZeroed(NumSoureChannels * InNumOutputChannels);
			}
			// If it's a 2D sound, then just get a new channel map appropriate for the new device channel count
			else
			{
				SourceInfo.ScratchChannelMap.Reset();
				MixerDevice->Get2DChannelMap(NumSoureChannels, InNumOutputChannels, SourceInfo.bIsCenterChannelOnly, SourceInfo.ScratchChannelMap);
			}
			SourceInfo.ChannelMapParam.SetChannelMap(SourceInfo.ScratchChannelMap);
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
				FSourceInfo& SourceInfo = SourceInfos[SourceId];

				if (SourceInfo.SourceEffectChainId == InSourceEffectChainId)
				{
					SourceInfo.bEffectTailsDone = !bPlayEffectChainTails;

					// Check to see if the chain didn't actually change
					TArray<FSoundEffectSource *>& ThisSourceEffectChain = SourceInfo.SourceEffects;
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
						InitData.NumSourceChannels = SourceInfo.NumInputChannels;

						if (SourceInfo.NumInputFrames != INDEX_NONE)
						{
							InitData.SourceDuration = (float)SourceInfo.NumInputFrames / AUDIO_SAMPLE_RATE;
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

		if (NumSourceWorkers > 0 && !DisableParallelSourceProcessingCvar)
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
			FSourceInfo& SourceInfo = SourceInfos[SourceId];

			GameThreadInfo.bIsDone[SourceId] = SourceInfo.bIsDone;
			GameThreadInfo.bEffectTailsDone[SourceId] = SourceInfo.bEffectTailsDone;
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
