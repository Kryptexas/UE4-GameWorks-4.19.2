// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "RuntimeAssetCachePrivatePCH.h"
#include "RuntimeAssetCache.h"
#include "RuntimeAssetCachePluginInterface.h"
#include "RuntimeAssetCacheFilesystemBackend.h"
#include "RuntimeAssetCacheBucket.h"
#include "RuntimeAssetCacheAsyncWorker.h"
#include "RuntimeAssetCacheEntryMetadata.h"

DEFINE_STAT(STAT_RAC_ASyncWaitTime);

FRuntimeAssetCache::FRuntimeAssetCache()
{
	FString BucketName;
	int32 BucketSize;

	TArray<FString> BucketNames;
	GConfig->GetArray(TEXT("RuntimeAssetCache"), TEXT("BucketConfigs"), BucketNames, GEngineIni);

	for (FString& Bucket : BucketNames)
	{
		if (FParse::Value(*Bucket, TEXT("Name="), BucketName))
		{
			BucketSize = FRuntimeAssetCacheBucket::DefaultBucketSize;
			FParse::Value(*Bucket, TEXT("Size="), BucketSize);
			FRuntimeAssetCacheBucket* BucketDesc = FRuntimeAssetCacheBackend::Get().PreLoadBucket(FName(*BucketName), BucketSize);
			Buckets.Add(FName(*BucketName), BucketDesc);
		}
	}
}

FRuntimeAssetCache::~FRuntimeAssetCache()
{
	for (auto& Bucket : Buckets)
	{
		delete Bucket.Value;
	}

	Buckets.Empty();
}

int32 FRuntimeAssetCache::GetCacheSize(FName Bucket) const
{
	return Buckets[Bucket]->GetSize();
}

uint32 FRuntimeAssetCache::GetAsynchronous(FRuntimeAssetCacheBuilderInterface* CacheBuilder)
{
	int32 Handle = GetNextHandle();

	/** Must return a valid handle */
	check(Handle != 0);
	
	/** Make sure task isn't processed twice. */
	check(!PendingTasks.Contains(Handle));

	FAsyncTask<FRuntimeAssetCacheAsyncWorker>* AsyncTask = new FAsyncTask<FRuntimeAssetCacheAsyncWorker>(CacheBuilder, &Buckets);

	{
		FScopeLock ScopeLock(&SynchronizationObject);
		PendingTasks.Add(Handle, AsyncTask);
	}
	AddToAsyncCompletionCounter(1);
	AsyncTask->StartBackgroundTask();
	return Handle;
}

bool FRuntimeAssetCache::GetSynchronous(FRuntimeAssetCacheBuilderInterface* CacheBuilder, TArray<uint8>& OutData)
{
	FAsyncTask<FRuntimeAssetCacheAsyncWorker>* AsyncTask = new FAsyncTask<FRuntimeAssetCacheAsyncWorker>(CacheBuilder, &Buckets);
	AddToAsyncCompletionCounter(1);
	AsyncTask->StartSynchronousTask();
	OutData = AsyncTask->GetTask().GetData();
	return AsyncTask->GetTask().RetrievedEntry();
}

bool FRuntimeAssetCache::ClearCache()
{
	/** Tell backend to clean itself up. */
	bool bResult = FRuntimeAssetCacheBackend::Get().ClearCache();

	if (!bResult)
	{
		return bResult;
	}

	/** If backend is cleaned up, clean up all buckets. */
	for (auto& Bucket : Buckets)
	{
		Bucket.Value->Reset();
	}

	return bResult;
}

bool FRuntimeAssetCache::ClearCache(FName Bucket)
{
	/** Tell backend to clean bucket up. */
	bool bResult = FRuntimeAssetCacheBackend::Get().ClearCache(Bucket);

	if (!bResult)
	{
		return bResult;
	}

	/** If backend is cleaned up, clean up all buckets. */
	for (auto& Bucket : Buckets)
	{
		Bucket.Value->Reset();
	}

	return bResult;
}

void FRuntimeAssetCache::AddToAsyncCompletionCounter(int32 Value)
{
	PendingTasksCounter.Add(Value);
	check(PendingTasksCounter.GetValue() >= 0);
}

void FRuntimeAssetCache::WaitAsynchronousCompletion(uint32 Handle)
{
	STAT(double ThisTime = 0);
	{
		SCOPE_SECONDS_COUNTER(ThisTime);
		FAsyncTask<FRuntimeAssetCacheAsyncWorker>* AsyncTask = NULL;
		{
			FScopeLock ScopeLock(&SynchronizationObject);
			AsyncTask = PendingTasks.FindRef(Handle);
		}
		check(AsyncTask);
		AsyncTask->EnsureCompletion();
	}
	INC_FLOAT_STAT_BY(STAT_RAC_ASyncWaitTime, (float)ThisTime);
}

bool FRuntimeAssetCache::GetAsynchronousResults(uint32 Handle, TArray<uint8>& OutData)
{
	FAsyncTask<FRuntimeAssetCacheAsyncWorker>* AsyncTask = NULL;
	{
		FScopeLock ScopeLock(&SynchronizationObject);
		PendingTasks.RemoveAndCopyValue(Handle, AsyncTask);
	}
	check(AsyncTask);
	if (!AsyncTask->GetTask().RetrievedEntry())
	{
		delete AsyncTask;
		return false;
	}
	OutData = AsyncTask->GetTask().GetData();
	delete AsyncTask;
	check(OutData.Num());
	return true;
}

bool FRuntimeAssetCache::PollAsynchronousCompletion(uint32 Handle)
{
	FAsyncTask<FRuntimeAssetCacheAsyncWorker>* AsyncTask = NULL;
	{
		FScopeLock ScopeLock(&SynchronizationObject);
		AsyncTask = PendingTasks.FindRef(Handle);
	}
	check(AsyncTask);
	return AsyncTask->IsDone();
}
