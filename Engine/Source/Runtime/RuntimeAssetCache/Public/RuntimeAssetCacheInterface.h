// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/** Forward declarations */
class FRuntimeAssetCacheBuilderInterface;

/**
* Interface for the Runtime Asset Cache. Cache is split into buckets to cache various assets separately.
* Bucket names and maximum sizes are configure via Engine.ini config file using the following syntax
* [RuntimeAssetCache]
* +BucketConfigs=(Name="<Plugin name>", Size=<Maximum bucket size in bytes>)
* This API is fully thread safe.
**/
class FRuntimeAssetCacheInterface
{
public:
	/**
	* Synchronously gets value from cache. If value is not found, builds entry using CacheBuilder and updates cache.
	* @param CacheBuilder Builder to produce cache key and in the event of a miss.
	* @param OutData Reference to serialized data.
	* @return true if the data was retrieved from the cache or the deriver built the data successfully. false can only occur if the plugin returns false.
	**/
	virtual bool GetSynchronous(FRuntimeAssetCacheBuilderInterface* CacheBuilder, TArray<uint8>& OutData) = 0;

	/**
	* Asynchronously checks the cache. If value is not found, builds entry using CacheBuilder and updates cache.
	* @param CacheBuilder Builder to produce cache key and in the event of a miss, return the data.
	* @return Handle to worker.
	**/
	virtual uint32 GetAsynchronous(FRuntimeAssetCacheBuilderInterface* CacheBuilder) = 0;

	/**
	* Gets cache size.
	* @param Bucket to check.
	* @return Maximum allowed bucket size.
	**/
	virtual int32 GetCacheSize(FName Bucket) const = 0;

	/**
	* Removes all cache entries.
	* @return true if cache was successfully cleaned.
	*/
	virtual bool ClearCache() = 0;

	/**
	* Removes all cache from given bucket.
	* @param Bucket Bucket to clean.
	* @return true if cache was successfully cleaned.
	*/
	virtual bool ClearCache(FName Bucket) = 0;

	/**
	* Waits until worker finishes execution.
	* @param Handle Worker handle to wait for.
	*/
	virtual void WaitAsynchronousCompletion(uint32 Handle) = 0;

	/**
	* Gets asynchronous query results.
	* @param Handle Worker handle to check.
	* @param OutData Reference to serialized cache entry.
	* @return True if successfully retrieved data, false otherwise.
	*/
	virtual bool GetAsynchronousResults(uint32 Handle, TArray<uint8>& OutData) = 0;

	/**
	* Checks if worker finished execution.
	* @param Handle Worker handle to check.
	* @return True if execution finished, false otherwise.
	*/
	virtual bool PollAsynchronousCompletion(uint32 Handle) = 0;

	/**
	 * Adds a number from the thread safe counter which tracks outstanding async requests. This is used to ensure everything is complete prior to shutdown.
	 * @param Value to add to counter. Can be negative.
	 */
	virtual void AddToAsyncCompletionCounter(int32 Addend) = 0;
};
