// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ImgMediaScheduler.h"

#include "GenericPlatform/GenericPlatformAffinity.h"
#include "HAL/PlatformProcess.h"
#include "Misc/ScopeLock.h"
#include "UObject/Class.h"
#include "UObject/UObjectGlobals.h"

#include "ImgMediaLoader.h"
#include "ImgMediaSchedulerThread.h"
#include "ImgMediaSettings.h"


/** Minimum stack size for worker threads. */
#define IMGMEDIA_SCHEDULERTHREAD_MIN_STACK_SIZE 128 * 1024


/* FImgMediaLoaderScheduler structors
 *****************************************************************************/

FImgMediaScheduler::FImgMediaScheduler()
	: LoaderRoundRobin(INDEX_NONE)
{ }


FImgMediaScheduler::~FImgMediaScheduler()
{
	Shutdown();
}


/* FImgMediaLoaderScheduler interface
 *****************************************************************************/

void FImgMediaScheduler::Initialize()
{
	if (AllThreads.Num() > 0)
	{
		return; // already initialized
	}

	int32 NumWorkers = GetDefault<UImgMediaSettings>()->CacheThreads;

	if (NumWorkers <= 0)
	{
		NumWorkers = FPlatformMisc::NumberOfWorkerThreadsToSpawn();
	}

	uint32 StackSize = GetDefault<UImgMediaSettings>()->CacheThreadStackSizeKB;

	if (StackSize < IMGMEDIA_SCHEDULERTHREAD_MIN_STACK_SIZE)
	{
		StackSize = IMGMEDIA_SCHEDULERTHREAD_MIN_STACK_SIZE;
	}

	// create worker threads
	FScopeLock Lock(&CriticalSection);

	while (NumWorkers-- > 0)
	{
		auto Thread = new FImgMediaSchedulerThread(*this, StackSize, TPri_Normal);
		AvailableThreads.Add(Thread);
		AllThreads.Add(Thread);
	}
}


void FImgMediaScheduler::RegisterLoader(const TSharedRef<FImgMediaLoader, ESPMode::ThreadSafe>& Loader)
{
	FScopeLock Lock(&CriticalSection);
	Loaders.Add(Loader);
}


void FImgMediaScheduler::Shutdown()
{
	FScopeLock Lock(&CriticalSection);

	// destroy worker threads
	for (FImgMediaSchedulerThread* Thread : AllThreads)
	{
		delete Thread;
	}

	AllThreads.Empty();
	AvailableThreads.Empty();

	LoaderRoundRobin = INDEX_NONE;
}


void FImgMediaScheduler::UnregisterLoader(const TSharedRef<FImgMediaLoader, ESPMode::ThreadSafe>& Loader)
{
	FScopeLock Lock(&CriticalSection);

	const int32 LoaderIndex = Loaders.Find(Loader);
	RemoveLoader(LoaderIndex);
}


void FImgMediaScheduler::RemoveLoader(int32 LoaderIndex)
{
	if (Loaders.IsValidIndex(LoaderIndex))
	{
		Loaders.RemoveAt(LoaderIndex);

		if (LoaderIndex <= LoaderRoundRobin)
		{
			--LoaderRoundRobin;
			check(LoaderRoundRobin >= INDEX_NONE);
		}
	}
}


/* IMediaClockSink interface
 *****************************************************************************/

void FImgMediaScheduler::TickFetch(FTimespan DeltaTime, FTimespan Timecode)
{
	FScopeLock Lock(&CriticalSection);

	while (AvailableThreads.Num() > 0)
	{
		IQueuedWork* Work = GetWorkOrReturnToPool(nullptr);

		if (Work == nullptr)
		{
			break;
		}

		FImgMediaSchedulerThread* Thread = AvailableThreads.Pop();
		check(Thread != nullptr);

		Thread->QueueWork(Work);
	}
}


/* IImgMediaScheduler interface
 *****************************************************************************/

IQueuedWork* FImgMediaScheduler::GetWorkOrReturnToPool(FImgMediaSchedulerThread* Thread)
{
	FScopeLock Lock(&CriticalSection);

	if ((Thread == nullptr) || FPlatformProcess::SupportsMultithreading())
	{
		const int32 NumLoaders = Loaders.Num();

		if (NumLoaders > 0)
		{
			int32 CheckedLoaders = 0;

			while (CheckedLoaders++ < NumLoaders)
			{
				LoaderRoundRobin = (LoaderRoundRobin + 1) % NumLoaders;

				TSharedPtr<FImgMediaLoader, ESPMode::ThreadSafe> Loader = Loaders[LoaderRoundRobin].Pin();

				if (Loader.IsValid())
				{
					IQueuedWork* Work = Loader->GetWork();

					if (Work != nullptr)
					{
						return Work;
					}
				}
				else
				{
					RemoveLoader(LoaderRoundRobin);
				}
			}
		}
	}

	if (Thread != nullptr)
	{
		AvailableThreads.Add(Thread);
	}

	return nullptr;
}
