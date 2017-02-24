// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

/* Public dependencies
*****************************************************************************/

#include "CoreMinimal.h"
#include "AudioMixerBuffer.h"
#include "AudioMixerSubmix.h"
#include "DSP/OnePole.h"
#include "DSP/EnvelopeFollower.h"
#include "IAudioExtensionPlugin.h"
#include "Containers/Queue.h"

#define ENABLE_AUDIO_OUTPUT_DEBUGGING 0

#if AUDIO_MIXER_ENABLE_DEBUG_MODE

// Macro which checks if the source id is in debug mode, avoids having a bunch of #ifdefs in code
#define AUDIO_MIXER_DEBUG_LOG(SourceId, Format, ...)																							\
	if (bIsDebugMode[SourceId])																													\
	{																																			\
		FString CustomMessage = FString::Printf(Format, ##__VA_ARGS__);																			\
		FString LogMessage = FString::Printf(TEXT("<Debug Sound Log> [Id=%d][Name=%s]: %s"), SourceId, *DebugName[SourceId], *CustomMessage);	\
		UE_LOG(LogAudioMixer, Log, TEXT("%s"), *LogMessage);																								\
	}

#else

#define AUDIO_MIXER_DEBUG_LOG(SourceId, Message)

#endif


namespace Audio
{
	class FMixerSubmix;
	class FMixerDevice;
	class FMixerSourceVoice;

	struct FMixerSourceVoiceBuffer;

	typedef TSharedPtr<FMixerSourceVoiceBuffer, ESPMode::ThreadSafe> FMixerSourceBufferPtr;
	typedef TSharedPtr<FMixerSubmix, ESPMode::ThreadSafe> FMixerSubmixPtr;

	class ISourceBufferQueueListener
	{
	public:
		// Called when the current buffer is finished and a new one needs to be queued
		virtual void OnSourceBufferEnd() = 0;

		// Called when the buffer queue listener is released. Allows cleaning up any resources from render thread.
		virtual void OnRelease() = 0;
	};

	struct FMixerSourceSubmixSend
	{
		FMixerSubmixPtr Submix;
		float DryLevel;
		float WetLevel;
	};

	struct FMixerSourceVoiceInitParams
	{
		ISourceBufferQueueListener* BufferQueueListener;
		TArray<FMixerSourceSubmixSend> SubmixSends;
		TArray<USoundEffectSourcePreset*> SourceEffectChain;
		FMixerSourceVoice* SourceVoice;
		int32 NumInputChannels;
		int32 NumInputFrames;
		FString DebugName;
		bool bPlayEffectChainTails;
		bool bUseHRTFSpatialization;
		bool bIsDebugMode;

		FMixerSourceVoiceInitParams()
			: BufferQueueListener(nullptr)
			, SourceVoice(nullptr)
			, NumInputChannels(0)
			, NumInputFrames(0)
			, bPlayEffectChainTails(true)
			, bUseHRTFSpatialization(false)
			, bIsDebugMode(false)
		{}
	};

	class FSourceParam
	{
	public:
		FSourceParam(float InNumInterpFrames);

		void Reset();

		void SetValue(float InValue);
		float Update();
		float GetValue() const;

	private:
		float StartValue;
		float EndValue;
		float CurrentValue;
		float NumInterpFrames;
		float Frame;
		bool bIsInit;
		bool bIsDone;
	};


	class FSourceChannelMap
	{
	public:
		FSourceChannelMap(float InNumInterpFrames)
			: NumInterpFrames(InNumInterpFrames)
		{}

		void Reset()
		{
			ChannelValues.Reset();
		}

		void SetChannelMap(const TArray<float>& ChannelMap)
		{
			if (ChannelValues.Num() != ChannelMap.Num())
			{
				ChannelValues.Reset();
				for (int32 i = 0; i < ChannelMap.Num(); ++i)
				{
					ChannelValues.Add(FSourceParam(NumInterpFrames));
					ChannelValues[i].SetValue(ChannelMap[i]);
				}
			}
			else
			{
				for (int32 i = 0; i < ChannelMap.Num(); ++i)
				{
					ChannelValues[i].SetValue(ChannelMap[i]);
				}
			}
		}

		void GetChannelMap(TArray<float>& OutChannelMap)
		{
			OutChannelMap.Reset();
			for (int32 i = 0; i < ChannelValues.Num(); ++i)
			{
				OutChannelMap.Add(ChannelValues[i].Update());
			}
		}

		void PadZeroes(const int32 ToSize)
		{
			int32 CurrentSize = ChannelValues.Num();
			for (int32 i = CurrentSize; i < ToSize; ++i)
			{
				ChannelValues.Add(FSourceParam(NumInterpFrames));
				ChannelValues[i].SetValue(0.0f);
			}
		}

		TArray<FSourceParam> ChannelValues;
		float NumInterpFrames;
	};

	struct FSourceManagerInitParams
	{
		// Total number of sources to use in the source manager
		int32 NumSources;

		// Number of worker threads to use for the source manager.
		int32 NumSourceWorkers;
	};

	class FMixerSourceManager
	{
	public:
		FMixerSourceManager(FMixerDevice* InMixerDevice);
		~FMixerSourceManager();

		void Init(const FSourceManagerInitParams& InitParams);
		void Update();

		bool GetFreeSourceId(int32& OutSourceId);
		int32 GetNumActiveSources() const;

		void ReleaseSourceId(const int32 SourceId);
		void InitSource(const int32 SourceId, const FMixerSourceVoiceInitParams& InitParams);

		void Play(const int32 SourceId);
		void Stop(const int32 SourceId);
		void Pause(const int32 SourceId);
		void SetPitch(const int32 SourceId, const float Pitch);
		void SetVolume(const int32 SourceId, const float Volume);
		void SetSpatializationParams(const int32 SourceId, const FSpatializationParams& InParams);
		void SetWetLevel(const int32 SourceId, const float WetLevel);
		void SetMasterReverbWetLevel(const int32 SourceId, const float MasterReverbWetLevel);
		void SetChannelMap(const int32 SourceId, const TArray<float>& InChannelMap, const bool bInIs3D, const bool bInIsCenterChannelOnly);
		void SetLPFFrequency(const int32 SourceId, const float Frequency);
		
		void SubmitBuffer(const int32 SourceId, FMixerSourceBufferPtr InSourceVoiceBuffer);
		void SubmitBufferAudioThread(const int32 SourceId, FMixerSourceBufferPtr InSourceVoiceBuffer);

		int64 GetNumFramesPlayed(const int32 SourceId) const;
		bool IsDone(const int32 SourceId) const;
		bool IsEffectTailsDone(const int32 SourceId) const;
		bool NeedsSpeakerMap(const int32 SourceId) const;
		void ComputeNextBlockOfSamples();
		void MixOutputBuffers(const int32 SourceId, TArray<float>& OutDryBuffer, TArray<float>& OutWetBuffer, const float DryLevel, const float WetLevel) const;

		void SetSubmixSendInfo(const int32 SourceId, FMixerSubmixPtr Submix, const float DryLevel, const float WetLevel);

		void UpdateDeviceChannelCount(const int32 InNumOutputChannels);

	private:

		void ReleaseSource(const int32 SourceId);
		void ReadSourceFrame(const int32 SourceId);
		void ComputeSourceBuffersForIdRange(const int32 SourceIdStart, const int32 SourceIdEnd);
		void ComputePostSourceEffectBufferForIdRange(const int32 SourceIdStart, const int32 SourceIdEnd);
		void ComputeOutputBuffersForIdRange(const int32 SourceIdStart, const int32 SourceIdEnd);

		void AudioMixerThreadCommand(TFunction<void()> InFunction);
		void PumpCommandQueue();

		static const int32 NUM_BYTES_PER_SAMPLE = 2;

		// Private class which perform source buffer processing in a worker task
		class FAudioMixerSourceWorker : public FNonAbandonableTask
		{
			FMixerSourceManager* SourceManager;
			int32 StartSourceId;
			int32 EndSourceId;

		public:
			FAudioMixerSourceWorker(FMixerSourceManager* InSourceManager, const int32 InStartSourceId, const int32 InEndSourceId)
				: SourceManager(InSourceManager)
				, StartSourceId(InStartSourceId)
				, EndSourceId(InEndSourceId)
			{
			}

			void DoWork()
			{
				// Get the next block of frames from the source buffers
				SourceManager->ComputeSourceBuffersForIdRange(StartSourceId, EndSourceId);

				// Compute the audio source buffers after their individual effect chain processing
				SourceManager->ComputePostSourceEffectBufferForIdRange(StartSourceId, EndSourceId);

				// Get the audio for the output buffers
				SourceManager->ComputeOutputBuffersForIdRange(StartSourceId, EndSourceId);
			}

			FORCEINLINE TStatId GetStatId() const
			{
				RETURN_QUICK_DECLARE_CYCLE_STAT(FAudioMixerSourceWorker, STATGROUP_ThreadPoolAsyncTasks);
			}
		};

		FMixerDevice* MixerDevice;

		// Array of pointers to game thread audio source objects
		TArray<FMixerSourceVoice*> MixerSources;

		// A command queue to execute commands from audio thread (or game thread) to audio mixer device thread.
		TQueue<TFunction<void()>> SourceCommandQueue;

		/////////////////////////////////////////////////////////////////////////////
		// Vectorized source voice info
		/////////////////////////////////////////////////////////////////////////////

		// Raw PCM buffer data
		TArray<TQueue<FMixerSourceBufferPtr>> BufferQueue;
		TArray<ISourceBufferQueueListener*> BufferQueueListener;
		TArray<FThreadSafeCounter> NumBuffersQueued;

		// Debugging source generators
#if ENABLE_AUDIO_OUTPUT_DEBUGGING
		TArray<FSineOsc> DebugOutputGenerators;
#endif
		// Array of sources which are currently debug solo'd
		TArray<int32> DebugSoloSources;
		TArray<bool> bIsDebugMode;
		TArray<FString> DebugName;

		// Data used for rendering sources
		TArray<FMixerSourceBufferPtr> CurrentPCMBuffer;
		TArray<int32> CurrentAudioChunkNumFrames;
		TArray<TArray<float>> SourceBuffer;
		TArray<TArray<float>> HRTFSourceBuffer;
		TArray<TArray<float>> CurrentFrameValues;
		TArray<TArray<float>> NextFrameValues;
		TArray<float> CurrentFrameAlpha;
		TArray<int32> CurrentFrameIndex;
		TArray<int64> NumFramesPlayed;

		TArray<FSourceParam> PitchSourceParam;
		TArray<FSourceParam> VolumeSourceParam;
		TArray<FSourceParam> LPFCutoffFrequencyParam;

		// Simple LPFs for all sources (all channels of all sources)
		TArray<TArray<FOnePoleLPF>> LowPassFilters;

		// Source effect instances
		TArray<TArray<FSoundEffectSource*>> SourceEffects;
		TArray<TArray<USoundEffectSourcePreset*>> SourceEffectPresets;
		TArray<bool> bEffectTailsDone;
		FSoundEffectSourceInputData SourceEffectInputData;
		FSoundEffectSourceOutputData SourceEffectOutputData;

		// A DSP object which tracks the amplitude envelope of a source.
		TArray<Audio::FEnvelopeFollower> SourceEnvelopeFollower;
		TArray<float> SourceEnvelopeValue;

		TArray<FSourceChannelMap> ChannelMapParam;
		TArray<FSpatializationParams> SpatParams;
		TArray<TArray<float>> ScratchChannelMap;

		// Output data, after computing a block of sample data, this is read back from mixers
		TArray<TArray<float>*> PostEffectBuffers;
		TArray<TArray<float>> OutputBuffer;

		// State management
		TArray<bool> bIs3D;
		TArray<bool> bIsCenterChannelOnly;
		TArray<bool> bIsActive;
		TArray<bool> bIsPlaying;
		TArray<bool> bIsPaused;
		TArray<bool> bHasStarted;
		TArray<bool> bIsBusy;
		TArray<bool> bUseHRTFSpatializer;
		TArray<bool> bIsDone;

		// Source format info
		TArray<int32> NumInputChannels;
		TArray<int32> NumPostEffectChannels;
		TArray<int32> NumInputFrames;

		// Async task workers for processing sources in parallel
		TArray<FAsyncTask<FAudioMixerSourceWorker>*> SourceWorkers;

		// General information about sources in source manager accessible from game thread
		struct FGameThreadInfo
		{
			TArray<int32> FreeSourceIndices;
			TArray<bool> bIsBusy;
			TArray<FThreadSafeBool> bIsDone;
			TArray<FThreadSafeBool> bEffectTailsDone;
			TArray <bool> bNeedsSpeakerMap;
			TArray<bool> bIsDebugMode;
		} GameThreadInfo;

		int32 NumActiveSources;
		int32 NumTotalSources;
		int32 NumSourceWorkers;
		uint8 bInitialized : 1;

		friend class FMixerSourceVoice;

		// Set to true when the audio source manager should pump the command queue
		FThreadSafeBool bPumpQueue;
	};
}
