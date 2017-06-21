// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AudioMixerSourceDecode.h"
#include "CoreMinimal.h"
#include "Stats/Stats.h"
#include "AudioMixer.h"
#include "Sound/SoundWave.h"
#include "HAL/RunnableThread.h"
#include "AudioMixerBuffer.h"
#include "Async/Async.h"

// Toggles using futures for audio tasks
#define AUDIO_DECODE_USE_FUTURES 0

namespace Audio
{

#if AUDIO_DECODE_USE_FUTURES

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

#else // #if AUDIO_DECODE_USE_FUTURES

class FAsyncDecodeWorker : public FNonAbandonableTask
{
public:
	FAsyncDecodeWorker(const FHeaderParseAudioTaskData& InTaskData)
		: HeaderParseAudioData(InTaskData)
		, TaskType(EAudioTaskType::Header)
		, bIsDone(false)
	{
	}

	FAsyncDecodeWorker(const FProceduralAudioTaskData& InTaskData)
		: ProceduralTaskData(InTaskData)
		, TaskType(EAudioTaskType::Procedural)
		, bIsDone(false)
	{
	}

	FAsyncDecodeWorker(const FDecodeAudioTaskData& InTaskData)
		: DecodeTaskData(InTaskData)
		, TaskType(EAudioTaskType::Decode)
		, bIsDone(false)
	{
	}

	void DoWork()
	{
		switch (TaskType)
		{
			case EAudioTaskType::Procedural:
			{
				ProceduralResult.NumBytesWritten = ProceduralTaskData.ProceduralSoundWave->GeneratePCMData(ProceduralTaskData.AudioData, ProceduralTaskData.MaxAudioDataSamples);
			}
			break;

			case EAudioTaskType::Header:
			{
				HeaderParseAudioData.MixerBuffer->ReadCompressedInfo(HeaderParseAudioData.SoundWave);
			}
			break;

			case EAudioTaskType::Decode:
			{
				// skip the first buffer if we've already decoded them
				if (DecodeTaskData.bSkipFirstBuffer)
				{
#if PLATFORM_ANDROID
					// Only skip one buffer on Android
					DecodeTaskData.MixerBuffer->ReadCompressedData(DecodeTaskData.AudioData, DecodeTaskData.NumFramesToDecode, DecodeTaskData.bLoopingMode);
#else // #if PLATFORM_ANDROID
					// If we're using cached data we need to skip the first two reads from the data
					DecodeTaskData.MixerBuffer->ReadCompressedData(DecodeTaskData.AudioData, DecodeTaskData.NumFramesToDecode, DecodeTaskData.bLoopingMode);
					DecodeTaskData.MixerBuffer->ReadCompressedData(DecodeTaskData.AudioData, DecodeTaskData.NumFramesToDecode, DecodeTaskData.bLoopingMode);
#endif // #else // #if PLATFORM_ANDROID
				}

				DecodeResult.bLooped = DecodeTaskData.MixerBuffer->ReadCompressedData(DecodeTaskData.AudioData, DecodeTaskData.NumFramesToDecode, DecodeTaskData.bLoopingMode);
			}
			break;
		}
		bIsDone = true;
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FAsyncDecodeWorker, STATGROUP_ThreadPoolAsyncTasks);
	}

	FHeaderParseAudioTaskData HeaderParseAudioData;
	FDecodeAudioTaskData DecodeTaskData;
	FDecodeAudioTaskResults DecodeResult;
	FProceduralAudioTaskData ProceduralTaskData;
	FProceduralAudioTaskResults ProceduralResult;
	EAudioTaskType TaskType;
	FThreadSafeBool bIsDone;
};

class FDecodeHandleBase : public IAudioTask
{
public:
	FDecodeHandleBase()
		: Task(nullptr)
	{}

	virtual ~FDecodeHandleBase()
	{
		if (Task)
		{
			Task->EnsureCompletion();
			delete Task;
		}
	}

	virtual bool IsDone() const override
	{
		if (Task)
		{
			return Task->IsDone();
		}
		return true;
	}

	virtual void EnsureCompletion() override
	{
		if (Task)
		{
			Task->EnsureCompletion();
		}
	}

protected:

	FAsyncTask<FAsyncDecodeWorker>* Task;
};

class FHeaderDecodeHandle : public FDecodeHandleBase
{
public:
	FHeaderDecodeHandle(const FHeaderParseAudioTaskData& InJobData)
	{
		Task = new FAsyncTask<FAsyncDecodeWorker>(InJobData);
		Task->StartBackgroundTask();
	}

	virtual EAudioTaskType GetType() const override
	{
		return EAudioTaskType::Header;
	}
};

class FProceduralDecodeHandle : public FDecodeHandleBase
{
public:
	FProceduralDecodeHandle(const FProceduralAudioTaskData& InJobData)
	{
		Task = new FAsyncTask<FAsyncDecodeWorker>(InJobData);
		Task->StartBackgroundTask();
	}

	virtual EAudioTaskType GetType() const override
	{ 
		return EAudioTaskType::Procedural; 
	}

	virtual void GetResult(FProceduralAudioTaskResults& OutResult) override
	{
		Task->EnsureCompletion();
		const FAsyncDecodeWorker& DecodeWorker = Task->GetTask();
		OutResult = DecodeWorker.ProceduralResult;
	}
};

class FDecodeHandle : public FDecodeHandleBase
{
public:
	FDecodeHandle(const FDecodeAudioTaskData& InJobData)
	{
		Task = new FAsyncTask<FAsyncDecodeWorker>(InJobData);
		Task->StartBackgroundTask();
	}

	virtual EAudioTaskType GetType() const override
	{ 
		return EAudioTaskType::Decode; 
	}

	virtual void GetResult(FDecodeAudioTaskResults& OutResult) override
	{
		Task->EnsureCompletion();
		const FAsyncDecodeWorker& DecodeWorker = Task->GetTask();
		OutResult = DecodeWorker.DecodeResult;
	}
};

IAudioTask* CreateAudioTask(const FProceduralAudioTaskData& InJobData)
{
	return new FProceduralDecodeHandle(InJobData);
}

IAudioTask* CreateAudioTask(const FHeaderParseAudioTaskData& InJobData)
{
	return new FHeaderDecodeHandle(InJobData);
}

IAudioTask* CreateAudioTask(const FDecodeAudioTaskData& InJobData)
{
	return new FDecodeHandle(InJobData);
}

#endif // #else // #if AUDIO_DECODE_USE_FUTURES

}
