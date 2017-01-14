// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AudioMixerSourceManager.h"
#include "AudioMixerSource.h"
#include "AudioMixerDevice.h"
#include "AudioMixerSourceVoice.h"
#include "AudioMixerSubmix.h"
#include "IAudioExtensionPlugin.h"

#define ONEOVERSHORTMAX (3.0517578125e-5f) // 1/32768

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
			bIsDone = false;
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


	/*************************************************************************
	* FMixerSourceManager
	**************************************************************************/

	FMixerSourceManager::FMixerSourceManager(FMixerDevice* InMixerDevice)
		: MixerDevice(InMixerDevice)
		, NumActiveSources(0)
		, NumTotalSources(0)
		, bInitialized(false)
	{
	}

	FMixerSourceManager::~FMixerSourceManager()
	{

	}

	void FMixerSourceManager::Init(const int32 InNumSources)
	{
		AUDIO_MIXER_CHECK(MixerDevice);
		AUDIO_MIXER_CHECK(MixerDevice->GetSampleRate() > 0);
		AUDIO_MIXER_CHECK(InNumSources > 0);

		if (bInitialized)
		{
			return;
		}

#if ENABLE_AUDIO_OUTPUT_DEBUGGING
		for (int32 i = 0; i < InNumSources; ++i)
		{
			DebugOutputGenerators.Add(FSineOsc(44100, 50 + i * 5, 0.5f));
		}
#endif

		MixerSources.Init(nullptr, InNumSources);
		BufferQueue.AddDefaulted(InNumSources);
		BufferQueueListener.Init(nullptr, InNumSources);
		NumBuffersQueued.AddDefaulted(InNumSources);
		CurrentPCMBuffer.Init(nullptr, InNumSources);
		CurrentAudioChunkNumFrames.AddDefaulted(InNumSources);
		SourceBuffer.AddDefaulted(InNumSources);
		HRTFSourceBuffer.AddDefaulted(InNumSources);
		CurrentFrameValues.AddDefaulted(InNumSources);
		NextFrameValues.AddDefaulted(InNumSources);
		CurrentFrameAlpha.AddDefaulted(InNumSources);
		CurrentFrameIndex.AddDefaulted(InNumSources);
		NumFramesPlayed.AddDefaulted(InNumSources);

		//int32 NumOutputFrames = MixerDevice->GetNumOutputFrames();
		PitchSourceParam.Init(FSourceParam(128), InNumSources);
		VolumeSourceParam.Init(FSourceParam(128), InNumSources);
		LPFCutoffFrequencyParam.Init(FSourceParam(0), InNumSources);
		ChannelMapParam.Init(FSourceChannelMap(256), InNumSources);
		SpatParams.AddDefaulted(InNumSources);
		LowPassFilters.AddDefaulted(InNumSources);

		PostEffectBuffers.AddDefaulted(InNumSources);
		OutputBuffer.AddDefaulted(InNumSources);
		bIs3D.AddDefaulted(InNumSources);
		bIsCenterChannelOnly.AddDefaulted(InNumSources);
		bIsActive.AddDefaulted(InNumSources);
		bIsPlaying.AddDefaulted(InNumSources);
		bIsPaused.AddDefaulted(InNumSources);
		bIsDone.AddDefaulted(InNumSources);
		bIsBusy.AddDefaulted(InNumSources);
		bUseHRTFSpatializer.AddDefaulted(InNumSources);
		bHasStarted.AddDefaulted(InNumSources);
		bIsDebugMode.AddDefaulted(InNumSources);
		DebugName.AddDefaulted(InNumSources);

		NumInputChannels.AddDefaulted(InNumSources);
		NumPostEffectChannels.AddDefaulted(InNumSources);
		NumInputFrames.AddDefaulted(InNumSources);

		GameThreadInfo.bIsBusy.AddDefaulted(InNumSources);
		GameThreadInfo.bIsDone.AddDefaulted(InNumSources);
		GameThreadInfo.bNeedsSpeakerMap.AddDefaulted(InNumSources);
		GameThreadInfo.bIsDebugMode.AddDefaulted(InNumSources);
		GameThreadInfo.FreeSourceIndices.Reset(InNumSources);
		for (int32 i = InNumSources - 1; i >= 0; --i)
		{
			GameThreadInfo.FreeSourceIndices.Add(i);
		}

		// Initialize the source buffer memory usage to max source scratch buffers (num frames times max source channels)
		const int32 NumFrames = MixerDevice->GetNumOutputFrames();
		for (int32 SourceId = 0; SourceId < NumTotalSources; ++SourceId)
		{
			SourceBuffer[SourceId].Reset(NumFrames * 8);
			HRTFSourceBuffer[SourceId].Reset(NumFrames * 2);
		}

		NumTotalSources = InNumSources;
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
		HRTFSourceBuffer[SourceId].Reset();
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
		bIsDone[SourceId] = false;
		bHasStarted[SourceId] = false;
		bIsDebugMode[SourceId] = false;
		DebugName[SourceId] = FString();
		NumInputChannels[SourceId] = 0;
		NumPostEffectChannels[SourceId] = 0;

		GameThreadInfo.bNeedsSpeakerMap[SourceId] = false;
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
			bUseHRTFSpatializer[SourceId] = InitParams.bUseHRTFSpatialization;

			BufferQueueListener[SourceId] = InitParams.BufferQueueListener;
			NumInputChannels[SourceId] = InitParams.NumInputChannels;
			NumInputFrames[SourceId] = InitParams.NumInputFrames;

			AUDIO_MIXER_CHECK(BufferQueue[SourceId].IsEmpty());

			// Initialize the number of per-source LPF filters based on input channels
			LowPassFilters[SourceId].AddDefaulted(InitParams.NumInputChannels);

			CurrentFrameValues[SourceId].Init(0.0f, InitParams.NumInputChannels);
			NextFrameValues[SourceId].Init(0.0f, InitParams.NumInputChannels);

			AUDIO_MIXER_CHECK(MixerSources[SourceId] == nullptr);
			MixerSources[SourceId] = InitParams.SourceVoice;

			// Loop through the source's sends and add this source to those submixes with the send info
			for (int32 i = 0; i < InitParams.SubmixSends.Num(); ++i)
			{
				const FMixerSourceSubmixSend& MixerSourceSend = InitParams.SubmixSends[i];
				MixerSourceSend.Submix->AddOrSetSourceVoice(InitParams.SourceVoice, MixerSourceSend.DryLevel, MixerSourceSend.WetLevel);
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
	 
	void FMixerSourceManager::SubmitBuffer(const int32 SourceId, FMixerSourceBufferPtr InSourceVoiceBuffer)
	{
		AUDIO_MIXER_CHECK(SourceId < NumTotalSources);
		AUDIO_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);
		AUDIO_MIXER_CHECK(InSourceVoiceBuffer->AudioBytes <= (uint32)InSourceVoiceBuffer->AudioData.Num());
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		AudioMixerThreadCommand([this, SourceId, InSourceVoiceBuffer]()
		{
			AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

			BufferQueue[SourceId].Enqueue(InSourceVoiceBuffer);
		});
	}


	void FMixerSourceManager::SetSubmixSendInfo(const int32 SourceId, FMixerSubmixPtr Submix, const float DryLevel, const float WetLevel)
	{
		AUDIO_MIXER_CHECK(SourceId < NumTotalSources);
		AUDIO_MIXER_CHECK(GameThreadInfo.bIsBusy[SourceId]);
		AUDIO_MIXER_CHECK_GAME_THREAD(MixerDevice);

		AudioMixerThreadCommand([this, SourceId, Submix, DryLevel, WetLevel]()
		{
			AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);
			Submix->AddOrSetSourceVoice(MixerSources[SourceId], DryLevel, WetLevel);
		});
	}

	void FMixerSourceManager::SubmitBufferAudioThread(const int32 SourceId, FMixerSourceBufferPtr InSourceVoiceBuffer)
	{
		AUDIO_MIXER_CHECK(SourceId < NumTotalSources);
		AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

		// Immediately push the source buffer onto the audio thread buffer queue.
		BufferQueue[SourceId].Enqueue(InSourceVoiceBuffer);
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

					CurrentFrameIndex[SourceId] = CurrentFrameIndex[SourceId] - CurrentAudioChunkNumFrames[SourceId];
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
				if (!bIsDone[SourceId])
				{
					AUDIO_MIXER_DEBUG_LOG(SourceId, TEXT("Is now done."));

					bIsDone[SourceId] = true;
					BufferQueue[SourceId].Empty();
					MixerSources[SourceId]->NumBuffersQueued.Set(0);
					CurrentFrameValues[SourceId].Reset();
					NextFrameValues[SourceId].Reset();
					CurrentPCMBuffer[SourceId] = nullptr;
				}
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

	void FMixerSourceManager::ComputeSourceBuffers()
	{
		AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

		SCOPE_CYCLE_COUNTER(STAT_AudioMixerSourceBuffers);

		const int32 NumFrames = MixerDevice->GetNumOutputFrames();

		// Local variable used for sample values
		float SampleValue = 0.0f;

		for (int32 SourceId = 0; SourceId < NumTotalSources; ++SourceId)
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

	void FMixerSourceManager::ComputePostSourceEffectBuffers()
	{
		AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

		SCOPE_CYCLE_COUNTER(STAT_AudioMixerSourceEffectBuffers);

		// First run each source buffer through it's default source effects (e.g. LPF)

		const int32 NumFrames = MixerDevice->GetNumOutputFrames();
		class IAudioSpatializationAlgorithm* SpatializeProcessor = MixerDevice->SpatializeProcessor;

		const bool bIsDebugModeEnabled = DebugSoloSources.Num() > 0;

		for (int32 SourceId = 0; SourceId < NumTotalSources; ++SourceId)
		{
			if (!bIsBusy[SourceId] || !bIsPlaying[SourceId] || bIsPaused[SourceId])
			{
				continue;
			}

			// Get the source buffer
			TArray<float>& Buffer = SourceBuffer[SourceId];

			// First apply the LPF filter (and any other default per-source effecct) to the source before spatializing
			FSourceParam& LPFFrequencyParam = LPFCutoffFrequencyParam[SourceId];
			const int32 NumInputChans = NumInputChannels[SourceId];
			int32 SampleIndex = 0;

			// Update the source LPF frequencies outside the frame loop
			for (int32 Channel = 0; Channel < NumInputChans; ++Channel)
			{
				const float LPFFreq = LPFFrequencyParam.Update();
				// Update the frequency
				LowPassFilters[SourceId][Channel].SetFrequency(LPFFreq);
			}

			// Update the volume
#if AUDIO_MIXER_ENABLE_DEBUG_MODE				
			const float CurrentVolumeValue = (bIsDebugModeEnabled && !bIsDebugMode[SourceId]) ? 0.0f : VolumeSourceParam[SourceId].Update();

#else
			const float CurrentVolumeValue = VolumeSourceParam[SourceId].Update();
#endif

			for (int32 Frame = 0; Frame < NumFrames; ++Frame)
			{

				for (int32 Channel = 0; Channel < NumInputChans; ++Channel)
				{
					SampleIndex = NumInputChans * Frame + Channel;

					// Process the source through the filter
					float SourceSample = Buffer[SampleIndex];
					
					SourceSample = LowPassFilters[SourceId][Channel].ProcessAudio(SourceSample);

					// Process the effect chain
					// TODO
					// SourceSample = MyEffect[SourceId][Channel](SourceSample);

					SourceSample *= CurrentVolumeValue;

					// Write back out the sample after the effect processing and volume scaling
					Buffer[SampleIndex] = SourceSample;
				}
			}

			// If the source has HRTF processing enabled, run it through the spatializer
			if (bUseHRTFSpatializer[SourceId])
			{
				AUDIO_MIXER_CHECK(SpatializeProcessor);
				AUDIO_MIXER_CHECK(NumInputChannels[SourceId] == 1);

				// Reset our scratch buffer and make sure it has enough data to hold 2 channels of interleaved data
				HRTFSourceBuffer[SourceId].Reset();			
				HRTFSourceBuffer[SourceId].AddZeroed(2 * NumFrames);

				FSpatializationParams& SourceSpatParams = SpatParams[SourceId];
				SpatializeProcessor->SetSpatializationParameters(SourceId, SourceSpatParams);
				SpatializeProcessor->ProcessSpatializationForVoice(SourceId, Buffer.GetData(), HRTFSourceBuffer[SourceId].GetData());

				// We are now a 2-channel file and should not be spatialized using normal 3d spatialization
				NumPostEffectChannels[SourceId] = 2;

				PostEffectBuffers[SourceId] = &HRTFSourceBuffer[SourceId];
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

	void FMixerSourceManager::ComputeOutputBuffers()
	{
		AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

		SCOPE_CYCLE_COUNTER(STAT_AudioMixerSourceOutputBuffers);

		const int32 NumFrames = MixerDevice->GetNumOutputFrames();
		const int32 NumOutputChannels = MixerDevice->GetNumDeviceChannels();

		// Reset the dry/wet buffers for all the sources
		const int32 NumOutputSamples = NumFrames * NumOutputChannels;

		for (int32 SourceId = 0; SourceId < NumTotalSources; ++SourceId)
		{
			OutputBuffer[SourceId].Reset();
			OutputBuffer[SourceId].AddZeroed(NumOutputSamples);
		}

		for (int32 SourceId = 0; SourceId < NumTotalSources; ++SourceId)
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
					ChannelMapParam[SourceId].GetChannelMap(ScratchChannelMap);
					AUDIO_MIXER_CHECK(ScratchChannelMap.Num() == PostEffectChannels * NumOutputChannels);

					for (int32 OutputChannel = 0; OutputChannel < NumOutputChannels; ++OutputChannel)
					{
						// Look up the channel map value (maps input channels to output channels) for the source
						// This is the step that either applies the spatialization algorithm or just maps a 2d sound
						const int32 ChannelMapIndex = NumOutputChannels * SourceChannel + OutputChannel;
						const float ChannelMapValue = ScratchChannelMap[ChannelMapIndex];

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

	void FMixerSourceManager::MixOutputBuffers(const int32 SourceId, TArray<float>& OutDryBuffer, TArray<float>& OutWetBuffer, const float DryLevel, const float WetLevel) const
	{
		AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);
		AUDIO_MIXER_CHECK(SourceId < NumTotalSources);

		if (DryLevel > 0.0f)
		{
			for (int32 SampleIndex = 0; SampleIndex < OutDryBuffer.Num(); ++SampleIndex)
			{
				OutDryBuffer[SampleIndex] += OutputBuffer[SourceId][SampleIndex] * DryLevel;
			}
		}

		if (WetLevel > 0.0f)
		{
			for (int32 SampleIndex = 0; SampleIndex < OutDryBuffer.Num(); ++SampleIndex)
			{
				OutWetBuffer[SampleIndex] += OutputBuffer[SourceId][SampleIndex] * WetLevel;
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

			ScratchChannelMap.Reset();
			const int32 NumSoureChannels = bUseHRTFSpatializer[SourceId] ? 2 : NumInputChannels[SourceId];

			// If this is a 3d source, then just zero out the channel map, it'll cause a temporary blip
			// but it should reset in the next tick
			if (bIs3D[SourceId])
			{
				GameThreadInfo.bNeedsSpeakerMap[SourceId] = true;
				ScratchChannelMap.AddZeroed(NumSoureChannels * InNumOutputChannels);
			}
			// If it's a 2D sound, then just get a new channel map appropriate for the new device channel count
			else
			{			
				ScratchChannelMap.Reset();
				MixerDevice->Get2DChannelMap(NumSoureChannels, InNumOutputChannels, bIsCenterChannelOnly[SourceId], ScratchChannelMap);
			}
			ChannelMapParam[SourceId].SetChannelMap(ScratchChannelMap);
		}
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

		// Get the next block of frames from the source buffers
		ComputeSourceBuffers();

		// Compute the audio source buffers after their individual effect chain processing
		ComputePostSourceEffectBuffers();

		// Get the audio for the output buffers
		ComputeOutputBuffers();

		// Update the game thread copy of source doneness
		for (int32 SourceId = 0; SourceId < NumTotalSources; ++SourceId)
		{
			GameThreadInfo.bIsDone[SourceId] = bIsDone[SourceId];
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
