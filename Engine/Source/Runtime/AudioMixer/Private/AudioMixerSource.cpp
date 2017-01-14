// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AudioMixerSource.h"
#include "AudioMixerDevice.h"
#include "AudioMixerSourceVoice.h"
#include "IAudioExtensionPlugin.h"
#include "Sound/AudioSettings.h"

namespace Audio
{

	FMixerSource::FMixerSource(FAudioDevice* InAudioDevice)
		: FSoundSource(InAudioDevice)
		, MixerDevice((FMixerDevice*)InAudioDevice)
		, MixerBuffer(nullptr)
		, MixerSourceVoice(nullptr)
		, AsyncRealtimeAudioTask(nullptr)
		, PendingReleaseBuffer(nullptr)
		, PendingReleaseRealtimeAudioTask(nullptr)
		, CurrentBuffer(0)
		, PreviousAzimuth(-1.0f)
		, bPlayedCachedBuffer(false)
		, bPlaying(false)
		, bLoopCallback(false)
		, bIsFinished(false)
		, bBuffersToFlush(false)
		, bResourcesNeedFreeing(false)
		, bEditorWarnedChangedSpatialization(false)
		, bUsingHRTFSpatialization(false)
		, bIs3D(false)
		, bDebugMode(false)
	{
		// Create the source voice buffers
		for (int32 i = 0; i < Audio::MAX_BUFFERS_QUEUED; ++i)
		{
			SourceVoiceBuffers.Add(FMixerSourceBufferPtr(new FMixerSourceVoiceBuffer()));
		}
	}

	FMixerSource::~FMixerSource()
	{
		FreeResources();
	}

	bool FMixerSource::Init(FWaveInstance* InWaveInstance)
	{
		AUDIO_MIXER_CHECK(MixerBuffer);
		AUDIO_MIXER_CHECK(MixerBuffer->IsRealTimeSourceReady());

		FSoundSource::InitCommon();

		// Get the number of frames before creating the buffer
		int32 NumFrames = INDEX_NONE;

		if (InWaveInstance->WaveData->DecompressionType != EDecompressionType::DTYPE_Procedural)
		{
			const int32 NumBytes = InWaveInstance->WaveData->RawPCMDataSize;
			NumFrames = NumBytes / (InWaveInstance->WaveData->NumChannels * sizeof(int16));
		}

		FSoundBuffer* SoundBuffer = static_cast<FSoundBuffer*>(MixerBuffer);
		if (SoundBuffer->NumChannels > 0)
		{
			SCOPE_CYCLE_COUNTER(STAT_AudioSourceInitTime);

			AUDIO_MIXER_CHECK(MixerDevice);
			MixerSourceVoice = MixerDevice->GetMixerSourceVoice(InWaveInstance, this, UseHRTSpatialization());
			if (!MixerSourceVoice)
			{
				return false;
			}

			// Initialize the source voice with the necessary format information
			FMixerSourceVoiceInitParams InitParams;
			InitParams.BufferQueueListener = this;
			InitParams.NumInputChannels = InWaveInstance->WaveData->NumChannels;
			InitParams.NumInputFrames = NumFrames;
			InitParams.SourceVoice = MixerSourceVoice;
			InitParams.bUseHRTFSpatialization = UseHRTSpatialization();

			if (InWaveInstance->SoundSubmixSends.Num())
			{
				for (int32 i = 0; i < InWaveInstance->SoundSubmixSends.Num(); ++i)
				{
					FMixerSourceSubmixSend SubmixSend;
					SubmixSend.Submix = MixerDevice->GetSubmixInstance(InWaveInstance->SoundSubmixSends[i].SoundSubmix);
					SubmixSend.WetLevel = InWaveInstance->SoundSubmixSends[i].WetLevel;
					SubmixSend.DryLevel = InWaveInstance->SoundSubmixSends[i].DryLevel;

					InitParams.SubmixSends.Add(SubmixSend);
				}
			}
			else
			{
				// Send the voice to the EQ submix if it's enabled
				const bool bIsEQDisabled = GetDefault<UAudioSettings>()->bDisableMasterEQ;
				if (!bIsEQDisabled && IsEQFilterApplied())
				{
					// Default the submix to use to use the master submix if none are set
					FMixerSourceSubmixSend SubmixSend;
					SubmixSend.Submix = MixerDevice->GetMasterEQSubmix();
					SubmixSend.WetLevel = 1.0f;
					SubmixSend.DryLevel = 0.0f;
					InitParams.SubmixSends.Add(SubmixSend);
				}
				else
				{
					// Default the submix to use to use the master submix if none are set
					FMixerSourceSubmixSend SubmixSend;
					SubmixSend.Submix = MixerDevice->GetMasterSubmix();
					SubmixSend.WetLevel = 1.0f;
					SubmixSend.DryLevel = 0.0f;
					InitParams.SubmixSends.Add(SubmixSend);
				}
			}

			// Check to see if this sound has been flagged to be in debug mode
#if AUDIO_MIXER_ENABLE_DEBUG_MODE
			InitParams.DebugName = InWaveInstance->GetName();

			bool bIsDebug = false;
			FString WaveInstanceName = WaveInstance->GetName();
			FString TestName = GEngine->GetAudioDeviceManager()->GetAudioMixerDebugSoundName();
			if (WaveInstanceName.Contains(TestName))
			{
				bDebugMode = true;
				InitParams.bIsDebugMode = bDebugMode;
			}
#endif

			// Whether or not we're 3D
			bIs3D = !UseHRTSpatialization() && WaveInstance->bUseSpatialization && SoundBuffer->NumChannels < 3;

			// We support reverb
			SetReverbApplied(true);

			if (MixerSourceVoice->Init(InitParams))
			{
				AUDIO_MIXER_CHECK(WaveInstance);
				if (WaveInstance->StartTime > 0.0f)
				{
					MixerBuffer->Seek(WaveInstance->StartTime);
				}

				const EBufferType::Type BufferType = MixerBuffer->GetType();
				switch (BufferType)
				{
					case EBufferType::PCM:
					case EBufferType::PCMPreview:
						SubmitPCMBuffers();
						break;

					case EBufferType::PCMRealTime:
					case EBufferType::Streaming:
						SubmitPCMRTBuffers();
						break;

					case EBufferType::Invalid:
						break;
				}

				bInitialized = true;

				ChannelMap.Reset();

				Update();

				return true;
			}
		}
		return false;
	}

	void FMixerSource::Update()
	{
		SCOPE_CYCLE_COUNTER(STAT_AudioUpdateSources);

		if (!WaveInstance || !MixerSourceVoice || Paused || !bInitialized)
		{
			return;
		}

		UpdatePitch();

		UpdateVolume();

		UpdateSpatialization();

		UpdateEffects();

		UpdateChannelMaps();

		FSoundSource::DrawDebugInfo();
	}

	bool FMixerSource::PrepareForInitialization(FWaveInstance* InWaveInstance)
	{
		// We are currently not supporting playing audio on a controller
		if (InWaveInstance->OutputTarget == EAudioOutputTarget::Controller)
		{
			return false;
		}

		// We are not initialized yet. We won't be until the sound file finishes loading and parsing the header.
		bInitialized = false;

		//  Reset so next instance will warn if algorithm changes in-flight
		bEditorWarnedChangedSpatialization = false;

		check(InWaveInstance);
		check(MixerBuffer == nullptr);
		check(AudioDevice);

		MixerBuffer = FMixerBuffer::Init(AudioDevice, InWaveInstance->WaveData, InWaveInstance->StartTime > 0.0f);
		if (MixerBuffer)
		{
			Buffer = MixerBuffer;
			WaveInstance = InWaveInstance;

			LPFFrequency = MAX_FILTER_FREQUENCY;
			LastLPFFrequency = FLT_MAX;
			bIsFinished = false;

			// Not all wave data types have PCM data size at this point (e.g. procedural sound waves)
			if (InWaveInstance->WaveData->RawPCMDataSize > 0)
			{
				int32 NumBytes = InWaveInstance->WaveData->RawPCMDataSize;
				NumTotalFrames = NumBytes / (Buffer->NumChannels * sizeof(int16));
				check(NumTotalFrames > 0);
			}

			// We succeeded in preparing the buffer for initialization, but we are not technically initialized yet.
			// If the buffer is asynchronously preparing a file-handle, we may not yet initialize the source.
			return true;
		}

		// Something went wrong with initializing the generator
		return false;
	}

	bool FMixerSource::IsPreparedToInit()
	{
		return MixerBuffer && MixerBuffer->IsRealTimeSourceReady();
	}

	void FMixerSource::Play()
	{
		if (!WaveInstance)
		{
			return;
		}

		// It's possible if Pause and Play are called while a sound is async initializing. In this case
		// we'll just not actually play the source here. Instead we'll call play when the sound finishes loading.
		if (MixerSourceVoice && bInitialized)
		{
			MixerSourceVoice->Play();
		}

		Paused = false;
		Playing = true;
		bBuffersToFlush = false;
		bLoopCallback = false;
	}

	void FMixerSource::Stop()
	{
		bInitialized = false;
		if (WaveInstance)
		{
			FScopeLock Lock(&RenderThreadCritSect);

			if (MixerSourceVoice && Playing)
			{
				MixerSourceVoice->Stop();
			}

			Paused = false;
			Playing = false;

			FreeResources();
		}

		FSoundSource::Stop();
	}

	void FMixerSource::Pause()
	{
		if (!WaveInstance)
		{
			return;
		}

		if (MixerSourceVoice)
		{
			MixerSourceVoice->Pause();
		}

		Paused = true;
	}

	bool FMixerSource::IsFinished()
	{
		// A paused source is not finished.
		if (Paused || !bInitialized)
		{
			return false;
		}

		if (WaveInstance && MixerSourceVoice)
		{
			if (bIsFinished)
			{
				WaveInstance->NotifyFinished();
				return true;
			}

			if (bLoopCallback && WaveInstance->LoopingMode == LOOP_WithNotification)
			{
				WaveInstance->NotifyFinished();
				bLoopCallback = false;
			}

			return false;
		}
		return true;
	}

	FString FMixerSource::Describe(bool bUseLongName)
	{
		return FString(TEXT("Stub"));
	}

	float FMixerSource::GetPlaybackPercent() const
	{
		if (NumTotalFrames > 0)
		{
			int64 NumFrames = MixerSourceVoice->GetNumFramesPlayed();
			AUDIO_MIXER_CHECK(NumTotalFrames > 0);
			return (float)NumFrames / NumTotalFrames;
		}
		else
		{
			// If we don't have any frames, that means it's a procedural sound wave, which means
			// that we're never going to have a playback percentage.
			return 0.0f;
		}
	}

	void FMixerSource::SubmitPCMBuffers()
	{
		if (!AudioDevice)
		{
			UE_LOG(LogAudioMixer, Error, TEXT("SubmitPCMBuffers: Audio device is nullptr"));
			return;
		}

		if (!MixerSourceVoice)
		{
			UE_LOG(LogAudioMixer, Error, TEXT("SubmitPCMBuffers: Source is nullptr"));
			return;
		}

		CurrentBuffer = 0;

		uint8* Data = nullptr;
		uint32 DataSize = 0;
		MixerBuffer->GetPCMData(&Data, &DataSize);

		// Only submit data if we've successfully loaded it
		if (!Data || !DataSize)
		{
			UE_LOG(LogAudioMixer, Error, TEXT("Failed to load PCM data from sound source %s"), *WaveInstance->GetName());
			return;
		}

		// Reset the data, copy it over
		SourceVoiceBuffers[0]->AudioData.Reset();
		SourceVoiceBuffers[0]->AudioData.AddDefaulted(DataSize);
		FMemory::Memcpy(SourceVoiceBuffers[0]->AudioData.GetData(), Data, DataSize);

		AUDIO_MIXER_CHECK(SourceVoiceBuffers[0]->AudioData.Num() == DataSize);

		// Set the size of the data. In general AudioBytes may not be the same as AudioData.Num() 
		SourceVoiceBuffers[0]->AudioBytes = DataSize;
		SourceVoiceBuffers[0]->bRealTimeBuffer = false;
		SourceVoiceBuffers[0]->LoopCount = (WaveInstance->LoopingMode != LOOP_Never) ? Audio::LOOP_FOREVER : 0;

		MixerSourceVoice->SubmitBuffer(SourceVoiceBuffers[0]);
	}

	void FMixerSource::SubmitPCMRTBuffers()
	{
		CurrentBuffer = 0;

		const uint32 BufferSize = MONO_PCM_BUFFER_SIZE * MixerBuffer->NumChannels;

		// Set up buffer areas to decompress audio into
		for (int32 i = 0; i < Audio::MAX_BUFFERS_QUEUED; ++i)
		{
			SourceVoiceBuffers[i]->AudioData.Reset();
			SourceVoiceBuffers[i]->AudioData.AddZeroed(BufferSize);
			SourceVoiceBuffers[i]->AudioBytes = BufferSize;
			SourceVoiceBuffers[i]->bRealTimeBuffer = true;
			SourceVoiceBuffers[i]->LoopCount = 0;
		}

		bPlayedCachedBuffer = false;
		bool bIsSeeking = (WaveInstance->StartTime > 0.0f);
		if (!bIsSeeking && WaveInstance->WaveData && WaveInstance->WaveData->CachedRealtimeFirstBuffer)
		{
			bPlayedCachedBuffer = true;
			FMemory::Memcpy(SourceVoiceBuffers[0]->AudioData.GetData(), WaveInstance->WaveData->CachedRealtimeFirstBuffer, BufferSize);
			FMemory::Memcpy(SourceVoiceBuffers[1]->AudioData.GetData(), WaveInstance->WaveData->CachedRealtimeFirstBuffer + BufferSize, BufferSize);
		}
		else
		{
			// If we didn't cache or if we're seeking, then immediately read the next 2 buffers synchronously
			ReadMorePCMRTData(0, EBufferReadMode::Synchronous);
			ReadMorePCMRTData(1, EBufferReadMode::Synchronous);
		}

		MixerSourceVoice->SubmitBuffer(SourceVoiceBuffers[0]);
		MixerSourceVoice->SubmitBuffer(SourceVoiceBuffers[1]);

		// Set the current buffer index to 2 after
		CurrentBuffer = 2;

		bResourcesNeedFreeing = true;
	}

	bool FMixerSource::ReadMorePCMRTData(const int32 BufferIndex, EBufferReadMode BufferReadMode, bool* OutLooped)
	{
		USoundWave* WaveData = WaveInstance->WaveData;

		if (WaveData && WaveData->bProcedural)
		{
			AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);
			const int32 MaxSamples = (MONO_PCM_BUFFER_SIZE * Buffer->NumChannels) / sizeof(int16);

			if (BufferReadMode == EBufferReadMode::Synchronous || WaveData->bCanProcessAsync == false)
			{
				const int32 BytesWritten = WaveData->GeneratePCMData(SourceVoiceBuffers[BufferIndex]->AudioData.GetData(), MaxSamples);
				SourceVoiceBuffers[BufferIndex]->AudioBytes = BytesWritten;
			}
			else
			{
				check(!AsyncRealtimeAudioTask);
				AsyncRealtimeAudioTask = new FAsyncRealtimeAudioTask(WaveData, SourceVoiceBuffers[BufferIndex]->AudioData.GetData(), MaxSamples);
				AsyncRealtimeAudioTask->StartBackgroundTask();
			}

			// Not looping
			return false;
		}
		else if (BufferReadMode == EBufferReadMode::Synchronous)
		{
			return MixerBuffer->ReadCompressedData(SourceVoiceBuffers[BufferIndex]->AudioData.GetData(), WaveInstance->LoopingMode != LOOP_Never);
		}
		else
		{
			check(!AsyncRealtimeAudioTask);
			AsyncRealtimeAudioTask = new FAsyncRealtimeAudioTask(MixerBuffer, SourceVoiceBuffers[BufferIndex]->AudioData.GetData(), WaveInstance->LoopingMode != LOOP_Never, BufferReadMode == EBufferReadMode::AsynchronousSkipFirstFrame);
			AsyncRealtimeAudioTask->StartBackgroundTask();

			// Not looping
			return false;
		}
	}

	void FMixerSource::SubmitRealTimeSourceData(bool bLooped)
	{
		AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

		// Have we reached the end of the sound
		if (bLooped)
		{
			switch (WaveInstance->LoopingMode)
			{
				case LOOP_Never:
					// Play out any queued buffers - once there are no buffers left, the state check at the beginning of IsFinished will fire
					bBuffersToFlush = true;
					break;

				case LOOP_WithNotification:
					// If we have just looped, and we are looping, send notification
					// This will trigger a WaveInstance->NotifyFinished() in the FXAudio2SoundSournce::IsFinished() function on main thread.
					bLoopCallback = true;
					break;

				case LOOP_Forever:
					// Let the sound loop indefinitely
					break;
			}
		}

		if (MixerSourceVoice && SourceVoiceBuffers[CurrentBuffer]->AudioData.Num() > 0)
		{
			MixerSourceVoice->SubmitBufferAudioThread(SourceVoiceBuffers[CurrentBuffer]);
		}
	}

	void FMixerSource::ProcessRealTimeSource(bool bBlockForData)
	{
		const bool bGetMoreData = bBlockForData || (AsyncRealtimeAudioTask == nullptr);
		if (AsyncRealtimeAudioTask)
		{
			const bool bTaskDone = AsyncRealtimeAudioTask->IsDone();
			if (bTaskDone || bBlockForData)
			{
				bool bLooped = false;
				if (!bTaskDone)
				{
					AsyncRealtimeAudioTask->EnsureCompletion();
				}

				switch (AsyncRealtimeAudioTask->GetTask().GetTaskType())
				{
					case ERealtimeAudioTaskType::Decompress:
						bLooped = AsyncRealtimeAudioTask->GetTask().GetBufferLooped();
						break;

					case ERealtimeAudioTaskType::Procedural:
						SourceVoiceBuffers[CurrentBuffer]->AudioBytes = AsyncRealtimeAudioTask->GetTask().GetBytesWritten();
						break;
				}

				delete AsyncRealtimeAudioTask;
				AsyncRealtimeAudioTask = nullptr;
				
				SubmitRealTimeSourceData(bLooped);
			}
		}

		if (bGetMoreData)
		{
			// Update the buffer index
			if (++CurrentBuffer > 2)
			{
				CurrentBuffer = 0;
			}

			EBufferReadMode DataReadMode;
			if (bPlayedCachedBuffer)
			{
				bPlayedCachedBuffer = false;
				DataReadMode = EBufferReadMode::AsynchronousSkipFirstFrame;
			}
			else
			{
				DataReadMode = (MixerBuffer->GetType() == EBufferType::Streaming ? EBufferReadMode::Synchronous : EBufferReadMode::Asynchronous);
			}
			const bool bLooped = ReadMorePCMRTData(CurrentBuffer, DataReadMode);

			// If this was a synchronous read, then immediately write it
			if (AsyncRealtimeAudioTask == nullptr)
			{
				SubmitRealTimeSourceData(bLooped);
			}
		}
	}

	void FMixerSource::OnSourceBufferEnd()
	{
		AUDIO_MIXER_CHECK_AUDIO_PLAT_THREAD(MixerDevice);

		FScopeLock Lock(&RenderThreadCritSect);

		if (Playing && MixerSourceVoice)
		{
			int32 BuffersQueued = MixerSourceVoice->GetNumBuffersQueued();

			const bool bIsRealTimeSource = MixerBuffer->IsRealTimeBuffer();

			if (BuffersQueued == 0 && (bBuffersToFlush || !bIsRealTimeSource))
			{
				bIsFinished = true;
			}
			else if (bIsRealTimeSource && !bBuffersToFlush && BuffersQueued <= (Audio::MAX_BUFFERS_QUEUED - 1))
			{
				ProcessRealTimeSource(BuffersQueued < (Audio::MAX_BUFFERS_QUEUED - 1));
			}
		}
	}

	void FMixerSource::OnRelease()
	{
		// This can only be freed from the audio render thread
		if (PendingReleaseRealtimeAudioTask)
		{
			PendingReleaseRealtimeAudioTask->EnsureCompletion();
			delete PendingReleaseRealtimeAudioTask;
			PendingReleaseRealtimeAudioTask = nullptr;

			if (PendingReleaseBuffer)
			{
				delete PendingReleaseBuffer;
				PendingReleaseBuffer = nullptr;
			}
		}
	}

	void FMixerSource::FreeResources()
	{
		if (MixerBuffer)
		{
			MixerBuffer->EnsureHeaderParseTaskFinished();
		}

		if (MixerSourceVoice)
		{
			// Hand off the ptr of the async task so it can be shutdown on the audio render thread.
			PendingReleaseRealtimeAudioTask = AsyncRealtimeAudioTask;

			// This will trigger FMixerSource::OnRelease from audio render thread.
			MixerSourceVoice->Release();
			MixerSourceVoice = nullptr;
		}

		if (bResourcesNeedFreeing)
		{
			// If we have a buffer, we can't delete until the async decoding task has been ensured to complete.
			if (Buffer)
			{
				check(Buffer->ResourceID == 0);
				PendingReleaseBuffer = Buffer;
			}

			CurrentBuffer = 0;
		}

		MixerBuffer = nullptr;
		AsyncRealtimeAudioTask = nullptr;
		Buffer = nullptr;
		bBuffersToFlush = false;
		bLoopCallback = false;
		bResourcesNeedFreeing = false;
	}

	void FMixerSource::UpdatePitch()
	{
		AUDIO_MIXER_CHECK(MixerBuffer);

		check(WaveInstance);

		Pitch = WaveInstance->Pitch;

		// Don't apply global pitch scale to UI sounds
		if (!WaveInstance->bIsUISound)
		{
			Pitch *= AudioDevice->GetGlobalPitchScale().GetValue();
		}

		Pitch = FMath::Clamp<float>(Pitch, AUDIO_MIXER_MIN_PITCH, AUDIO_MIXER_MAX_PITCH);

		// Scale in the sound sample rate divided by device sample rate so pitch is 
		// accurate independent of sound source sample rate or device sample rate
		Pitch *= (MixerBuffer->GetSampleRate() / AudioDevice->GetSampleRate());

		MixerSourceVoice->SetPitch(Pitch);
	}

	void FMixerSource::UpdateVolume()
	{
		float CurrentVolume = 0.0f;

		if (!AudioDevice->IsAudioDeviceMuted())
		{
			CurrentVolume = WaveInstance->GetActualVolume();
		}

		CurrentVolume = FMath::Clamp<float>(CurrentVolume * AudioDevice->GetPlatformAudioHeadroom(), 0.0f, MAX_VOLUME);

		MixerSourceVoice->SetVolume(CurrentVolume);
	}

	void FMixerSource::UpdateSpatialization()
	{
		SpatializationParams = GetSpatializationParams();
		if (WaveInstance->bUseSpatialization)
		{
			MixerSourceVoice->SetSpatializationParams(SpatializationParams);
		}
	}

	void FMixerSource::UpdateEffects()
	{
		// Update the default LPF filter frequency 
		SetFilterFrequency();

		if (LastLPFFrequency != LPFFrequency)
		{
			MixerSourceVoice->SetLPFFrequency(LPFFrequency);
			LastLPFFrequency = LPFFrequency;
		}

		if (bReverbApplied)
		{
			// Get fraction of the sound to the 
			if (WaveInstance->bUseSpatialization && WaveInstance->ReverbDistanceMax > WaveInstance->ReverbDistanceMin)
			{
				float Alpha = (WaveInstance->ListenerToSoundDistance - WaveInstance->ReverbDistanceMin) / (WaveInstance->ReverbDistanceMax - WaveInstance->ReverbDistanceMin);
				Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
				float WetLevel = FMath::Lerp(WaveInstance->ReverbWetLevelMin, WaveInstance->ReverbWetLevelMax, Alpha);
				FMixerSubmixPtr MasterReverbSubmix = MixerDevice->GetMasterReverbSubmix();
				MixerSourceVoice->SetSubmixSendInfo(MasterReverbSubmix, 0.0f, WetLevel);
			}
			else
			{
				// If we're not 3d spatializing a sound, then use the default send amount, which is
				// set to ReverbWetLevelMin in the WaveInstance initialization
				FMixerSubmixPtr MasterReverbSubmix = MixerDevice->GetMasterReverbSubmix();
				MixerSourceVoice->SetSubmixSendInfo(MasterReverbSubmix, 0.0f, WaveInstance->ReverbWetLevelMin);
			}
		}
	}

	void FMixerSource::UpdateChannelMaps()
	{
		SetStereoBleed();

		SetLFEBleed();

		bool bChanged = false;

		check(Buffer);
		bChanged = ComputeChannelMap(Buffer->NumChannels);

		if (bChanged)
		{
			MixerSourceVoice->SetChannelMap(ChannelMap, bIs3D, WaveInstance->bCenterChannelOnly);
		}
	}

	bool FMixerSource::ComputeMonoChannelMap()
	{
		if (UseHRTSpatialization())
		{
			if (WaveInstance->SpatializationAlgorithm != SPATIALIZATION_HRTF && !bEditorWarnedChangedSpatialization)
			{
				bEditorWarnedChangedSpatialization = true;
				UE_LOG(LogAudioMixer, Warning, TEXT("Changing the spatialization algorithm on a playing sound is not supported (WaveInstance: %s)"), *WaveInstance->WaveData->GetFullName());
			}

			// Treat the source as if it is a 2D stereo source
			return ComputeStereoChannelMap();
		}
		else if (WaveInstance->bUseSpatialization && (!FMath::IsNearlyEqual(WaveInstance->AbsoluteAzimuth, PreviousAzimuth, 0.01f) || MixerSourceVoice->NeedsSpeakerMap()))
		{
			// Don't need to compute the source channel map if the absolute azimuth hasn't changed much
			PreviousAzimuth = WaveInstance->AbsoluteAzimuth;
			ChannelMap.Reset();
			MixerDevice->Get3DChannelMap(WaveInstance, WaveInstance->AbsoluteAzimuth, SpatializationParams.NormalizedOmniRadius, ChannelMap);
			return true;
		}
		else if (!ChannelMap.Num())
		{
			// Only need to compute the 2D channel map once
			MixerDevice->Get2DChannelMap(1, MixerDevice->GetNumDeviceChannels(), WaveInstance->bCenterChannelOnly, ChannelMap);
			return true;
		}

		// Return false means the channel map hasn't changed
		return false;
	}

	bool FMixerSource::ComputeStereoChannelMap()
	{
		if (!UseHRTSpatialization() && WaveInstance->bUseSpatialization && (!FMath::IsNearlyEqual(WaveInstance->AbsoluteAzimuth, PreviousAzimuth, 0.01f) || MixerSourceVoice->NeedsSpeakerMap()))
		{
			// Make sure our stereo emitter positions are updated relative to the sound emitter position
			UpdateStereoEmitterPositions();

			float AzimuthOffset = 0.0f;
			if (WaveInstance->ListenerToSoundDistance > 0.0f)
			{
				AzimuthOffset = FMath::Atan(0.5f * WaveInstance->StereoSpread / WaveInstance->ListenerToSoundDistance);
				AzimuthOffset = FMath::RadiansToDegrees(AzimuthOffset);
			}

			float LeftAzimuth = WaveInstance->AbsoluteAzimuth - AzimuthOffset;
			if (LeftAzimuth < 0.0f)
			{
				LeftAzimuth += 360.0f;
			}

			float RightAzimuth = WaveInstance->AbsoluteAzimuth + AzimuthOffset;
			if (RightAzimuth > 360.0f)
			{
				RightAzimuth -= 360.0f;
			}

			// Reset the channel map, the stereo spatialization channel mapping calls below will append their mappings
			ChannelMap.Reset();

			MixerDevice->Get3DChannelMap(WaveInstance, LeftAzimuth, SpatializationParams.NormalizedOmniRadius, ChannelMap);
			MixerDevice->Get3DChannelMap(WaveInstance, RightAzimuth, SpatializationParams.NormalizedOmniRadius, ChannelMap);

			int32 NumDeviceChannels = MixerDevice->GetNumDeviceChannels();
			check(ChannelMap.Num() == 2 * NumDeviceChannels);
			return true;
		}
		else if (!ChannelMap.Num())
		{
			MixerDevice->Get2DChannelMap(2, MixerDevice->GetNumDeviceChannels(), WaveInstance->bCenterChannelOnly, ChannelMap);
			return true;
		}

		return false;
	}

	bool FMixerSource::ComputeChannelMap(const int32 NumChannels)
	{
		if (NumChannels == 1)
		{
			return ComputeMonoChannelMap();
		}
		else if (NumChannels == 2)
		{
			return ComputeStereoChannelMap();
		}
		else if (!ChannelMap.Num())
		{
			MixerDevice->Get2DChannelMap(NumChannels, MixerDevice->GetNumDeviceChannels(), WaveInstance->bCenterChannelOnly, ChannelMap);
			return true;
		}
		return false;
	}

	bool FMixerSource::UseHRTSpatialization() const
	{
		return (Buffer->NumChannels == 1 &&
				AudioDevice->SpatializationPlugin != nullptr &&
				AudioDevice->IsSpatializationPluginEnabled() &&
				WaveInstance->SpatializationAlgorithm != SPATIALIZATION_Default);
	}



}