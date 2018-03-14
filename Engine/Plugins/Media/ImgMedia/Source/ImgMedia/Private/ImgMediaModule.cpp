// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ImgMediaPrivate.h"

#include "IMediaClock.h"
#include "IMediaModule.h"
#include "Misc/QueuedThreadPool.h"
#include "Modules/ModuleManager.h"

#include "ImgMediaPlayer.h"
#include "ImgMediaScheduler.h"
#include "IImgMediaModule.h"


DEFINE_LOG_CATEGORY(LogImgMedia);


#if USE_IMGMEDIA_DEALLOC_POOL
	FQueuedThreadPool* GImgMediaThreadPoolSlow = nullptr;
#endif


/**
 * Implements the AVFMedia module.
 */
class FImgMediaModule
	: public IImgMediaModule
{
public:

	/** Default constructor. */
	FImgMediaModule() { }

public:

	//~ IImgMediaModule interface

	virtual TSharedPtr<IMediaPlayer, ESPMode::ThreadSafe> CreatePlayer(IMediaEventSink& EventSink) override
	{
		return MakeShared<FImgMediaPlayer, ESPMode::ThreadSafe>(EventSink, Scheduler.ToSharedRef());
	}

public:

	//~ IModuleInterface interface

	virtual void StartupModule() override
	{
		// initialize scheduler
		Scheduler = MakeShared<FImgMediaScheduler, ESPMode::ThreadSafe>();
		Scheduler->Initialize();

		IMediaModule* MediaModule = FModuleManager::LoadModulePtr<IMediaModule>("Media");

		if (MediaModule != nullptr)
		{
			MediaModule->GetClock().AddSink(Scheduler.ToSharedRef());
		}

		// initialize worker thread pools
		if (FPlatformProcess::SupportsMultithreading())
		{
#if USE_IMGMEDIA_DEALLOC_POOL
			// initialize dealloc thread pool
			const int32 ThreadPoolSize = FPlatformProperties::IsServerOnly() ? 1 : FPlatformMisc::NumberOfWorkerThreadsToSpawn();
			const uint32 StackSize = 128 * 1024;

			GImgMediaThreadPoolSlow = FQueuedThreadPool::Allocate();
			verify(GImgMediaThreadPoolSlow->Create(ThreadPoolSize, StackSize, TPri_Normal));
#endif
		}
	}

	virtual void ShutdownModule() override
	{
		Scheduler.Reset();

#if USE_IMGMEDIA_DEALLOC_POOL
		if (GImgMediaThreadPoolSlow != nullptr)
		{
			GImgMediaThreadPoolSlow->Destroy();
			GImgMediaThreadPoolSlow = nullptr;
		}
#endif
	}

private:

	TSharedPtr<FImgMediaScheduler, ESPMode::ThreadSafe> Scheduler;
};


IMPLEMENT_MODULE(FImgMediaModule, ImgMedia);
