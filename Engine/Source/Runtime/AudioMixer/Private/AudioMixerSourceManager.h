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
	if (SourceInfos[SourceId].bIsDebugMode)																													\
	{																																			\
		FString CustomMessage = FString::Printf(Format, ##__VA_ARGS__);																			\
		FString LogMessage = FString::Printf(TEXT("<Debug Sound Log> [Id=%d][Name=%s]: %s"), SourceId, *SourceInfos[SourceId].DebugName, *CustomMessage);	\
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
		// The submix ptr
		FMixerSubmixPtr Submix;

		// The amount of audio that is to be mixed into this submix
		float SendLevel;

		// Whather or not this is the primary send (i.e. first in the send chain)
		bool bIsMainSend;
	};

	struct FMixerSourceVoiceInitParams
	{
		ISourceBufferQueueListener* BufferQueueListener;
		TArray<FMixerSourceSubmixSend> SubmixSends;
		uint32 SourceEffectChainId;
		TArray<FSourceEffectChainEntry> SourceEffectChain;
		FMixerSourceVoice* SourceVoice;
		int32 NumInputChannels;
		int32 NumInputFrames;
		FString DebugName;
		USpatializationPluginSourceSettingsBase* SpatializationPluginSettings;
		UOcclusionPluginSourceSettingsBase* OcclusionPluginSettings;
		UReverbPluginSourceSettingsBase* ReverbPluginSettings;
		FName AudioComponentUserID;
		bool bPlayEffectChainTails;
		bool bUseHRTFSpatialization;
		bool bIsDebugMode;

		FMixerSourceVoiceInitParams()
			: BufferQueueListener(nullptr)
			, SourceEffectChainId(INDEX_NONE)
			, SourceVoice(nullptr)
			, NumInputChannels(0)
			, NumInputFrames(0)
			, SpatializationPluginSettings(nullptr)
			, OcclusionPluginSettings(nullptr)
			, ReverbPluginSettings(nullptr)
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

		FORCEINLINE float GetCurrentValue() const { return CurrentValue; }

	private:
		float StartValue;
		float EndValue;
		float CurrentValue;
		float NumInterpFrames;
		float NumInterpFramesInverse;
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
			const int32 NumChannelValues = ChannelValues.Num();
			OutChannelMap.Reset();
			OutChannelMap.AddUninitialized(NumChannelValues);
			for (int32 i = 0; i < NumChannelValues; ++i)
			{
				OutChannelMap[i] = ChannelValues[i].Update();
			}
		}

		FORCEINLINE_DEBUGGABLE
		void UpdateChannelMap()
		{
			const int32 NumChannelValues = ChannelValues.Num();
			for (int32 i = 0; i < NumChannelValues; ++i)
			{
				ChannelValues[i].Update();
			}
		}

		FORCEINLINE_DEBUGGABLE
		float GetChannelValue(int ChannelIndex)
		{
			return ChannelValues[ChannelIndex].GetCurrentValue();
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
		void SetChannelMap(const int32 SourceId, const TArray<float>& InChannelMap, const bool bInIs3D, const bool bInIsCenterChannelOnly);
		void SetLPFFrequency(const int32 SourceId, const float Frequency);
		
		void SubmitBuffer(const int32 SourceId, FMixerSourceBufferPtr InSourceVoiceBuffer, const bool bSubmitSynchronously);

		int64 GetNumFramesPlayed(const int32 SourceId) const;
		bool IsDone(const int32 SourceId) const;
		bool IsEffectTailsDone(const int32 SourceId) const;
		bool NeedsSpeakerMap(const int32 SourceId) const;
		void ComputeNextBlockOfSamples();
		void MixOutputBuffers(const int32 SourceId, TArray<float>& OutWetBuffer, const float SendLevel) const;

		void SetSubmixSendInfo(const int32 SourceId, FMixerSubmixPtr Submix, const float SendLevel);

		void UpdateDeviceChannelCount(const int32 InNumOutputChannels);

		void UpdateSourceEffectChain(const uint32 SourceEffectChainId, const TArray<FSourceEffectChainEntry>& SourceEffectChain, const bool bPlayEffectChainTails);

	private:

		void ReleaseSource(const int32 SourceId);
		void BuildSourceEffectChain(const int32 SourceId, FSoundEffectSourceInitData& InitData, const TArray<FSourceEffectChainEntry>& SourceEffectChain);
		void ResetSourceEffectChain(const int32 SourceId);
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

		TArray<int32> DebugSoloSources;

		// Audio plugin processing
		FAudioPluginSourceInputData AudioPluginInputData;

		struct FSourceInfo
		{
			FSourceInfo();
			~FSourceInfo();

			// Raw PCM buffer data
			TQueue<FMixerSourceBufferPtr> BufferQueue;
			ISourceBufferQueueListener* BufferQueueListener;

			// Data used for rendering sources
			FMixerSourceBufferPtr CurrentPCMBuffer;
			int32 CurrentAudioChunkNumFrames;
			TArray<float> SourceBuffer;
			TArray<float> CurrentFrameValues;
			TArray<float> NextFrameValues;
			float CurrentFrameAlpha;
			int32 CurrentFrameIndex;
			int64 NumFramesPlayed;

			FSourceParam PitchSourceParam;
			FSourceParam VolumeSourceParam;
			FSourceParam LPFCutoffFrequencyParam;

			// Simple LPFs for all sources (all channels of all sources)
			TArray<FOnePoleLPF> LowPassFilters;

			// Source effect instances
			uint32 SourceEffectChainId;
			TArray<FSoundEffectSource*> SourceEffects;
			TArray<USoundEffectSourcePreset*> SourceEffectPresets;
			bool bEffectTailsDone;
			FSoundEffectSourceInputData SourceEffectInputData;
			FSoundEffectSourceOutputData SourceEffectOutputData;

			FAudioPluginSourceOutputData AudioPluginOutputData;

			// A DSP object which tracks the amplitude envelope of a source.
			Audio::FEnvelopeFollower SourceEnvelopeFollower;
			float SourceEnvelopeValue;

			FSourceChannelMap ChannelMapParam;
			FSpatializationParams SpatParams;
			TArray<float> ScratchChannelMap;

			// Output data, after computing a block of sample data, this is read back from mixers
			TArray<float> ReverbPluginOutputBuffer;
			TArray<float>* PostEffectBuffers;
			TArray<float> OutputBuffer;

			// State management
			bool bIs3D;
			bool bIsCenterChannelOnly;
			bool bIsActive;
			bool bIsPlaying;
			bool bIsPaused;
			bool bHasStarted;
			bool bIsBusy;
			bool bUseHRTFSpatializer;
			bool bUseOcclusionPlugin;
			bool bUseReverbPlugin;
			bool bIsDone;

			bool bIsDebugMode;
			FString DebugName;

			// Source format info
			int32 NumInputChannels;
			int32 NumPostEffectChannels;
			int32 NumInputFrames;
		};

		// Array of source infos.
		TArray<FSourceInfo> SourceInfos;

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
		int32 NumOutputFrames;
		int32 NumOutputSamples;
		int32 NumSourceWorkers;

		uint8 bInitialized : 1;

		friend class FMixerSourceVoice;
		// Set to true when the audio source manager should pump the command queue
		FThreadSafeBool bPumpQueue;
	};
}
