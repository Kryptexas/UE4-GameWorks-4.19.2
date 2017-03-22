// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AudioMixerSourceDecode.h"
#include "CoreMinimal.h"
#include "Stats/Stats.h"
#include "AudioMixer.h"
#include "Sound/SoundWave.h"
#include "HAL/RunnableThread.h"
#include "AudioMixerBuffer.h"
#include "Async/Async.h"

namespace Audio
{
	class FDecodeHandleBase : public IAudioTask
	{
	public:
		FDecodeHandleBase()
			: bFinished(false)
		{}

		virtual ~FDecodeHandleBase()
		{
			EnsureCompletion();
		}

		virtual bool IsDone() const override
		{
			return bFinished;
		}

		virtual void EnsureCompletion() override
		{
			Future.Get();
		}

	protected:
		FThreadSafeBool bFinished;
		TFuture<void> Future;
	};

	class FHeaderDecodeHandle : public FDecodeHandleBase
	{
	public:
		virtual EAudioTaskType GetType() const override { return EAudioTaskType::Header; }

		void Decode(const FHeaderParseAudioTaskData& InJobData)
		{
			TFunction<void()> Func = [this, InJobData]()
			{
				InJobData.MixerBuffer->ReadCompressedInfo(InJobData.SoundWave);
				bFinished = true;
			};

			Future = Async(EAsyncExecution::TaskGraph, Func);
		}
	};

	class FProceduralDecodeHandle : public FDecodeHandleBase
	{
	public:
		virtual EAudioTaskType GetType() const override { return EAudioTaskType::Procedural; }

		void Decode(const FProceduralAudioTaskData& InJobData)
		{
			TFunction<void()> Func = [this, InJobData]()
			{
				Result.NumBytesWritten = InJobData.ProceduralSoundWave->GeneratePCMData(InJobData.AudioData, InJobData.MaxAudioDataSamples);
				bFinished = true;
			};

			Future = Async(EAsyncExecution::TaskGraph, Func);
		}

		virtual void GetResult(FProceduralAudioTaskResults& OutResult) override
		{
			EnsureCompletion();
			OutResult = Result;
		}

	private:
		FProceduralAudioTaskResults Result;
	};

	class FDecodeHandle : public FDecodeHandleBase
	{
	public:
		virtual EAudioTaskType GetType() const override { return EAudioTaskType::Decode; }

		void Decode(const FDecodeAudioTaskData& InJobData)
		{

			TFunction<void()> Func = [this, InJobData]()
			{
				// skip the first buffer if we've already decoded them
				if (InJobData.bSkipFirstBuffer)
				{
#if PLATFORM_ANDROID
					// Only skip one buffer on Android
					InJobData.MixerBuffer->ReadCompressedData(InJobData.AudioData, InJobData.NumFramesToDecode, InJobData.bLoopingMode);
#else
					// If we're using cached data we need to skip the first two reads from the data
					InJobData.MixerBuffer->ReadCompressedData(InJobData.AudioData, InJobData.NumFramesToDecode, InJobData.bLoopingMode);
					InJobData.MixerBuffer->ReadCompressedData(InJobData.AudioData, InJobData.NumFramesToDecode, InJobData.bLoopingMode);
#endif
				}

				Result.bLooped = InJobData.MixerBuffer->ReadCompressedData(InJobData.AudioData, InJobData.NumFramesToDecode, InJobData.bLoopingMode);
				bFinished = true;
			};

			Future = Async(EAsyncExecution::TaskGraph, Func);
		}

		virtual void GetResult(FDecodeAudioTaskResults& OutResult) override
		{
			EnsureCompletion();
			OutResult = Result;
		}

	private:
		FDecodeAudioTaskResults Result;
	};

	IAudioTask* CreateAudioTask(const FProceduralAudioTaskData& InJobData)
	{
		FProceduralDecodeHandle* NewTask = new FProceduralDecodeHandle();
		NewTask->Decode(InJobData);
		return NewTask;
	}

	IAudioTask* CreateAudioTask(const FHeaderParseAudioTaskData& InJobData)
	{
		FHeaderDecodeHandle* NewTask = new FHeaderDecodeHandle();
		NewTask->Decode(InJobData);
		return NewTask;
	}

	IAudioTask* CreateAudioTask(const FDecodeAudioTaskData& InJobData)
	{
		FDecodeHandle* NewTask = new FDecodeHandle();
		NewTask->Decode(InJobData);
		return NewTask;
	}
}
