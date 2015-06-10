// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AsyncWork.h"

/** Stats */
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("RAC Num Build"), STAT_RAC_NumBuilds, STATGROUP_RAC, );
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("RAC Num Cache Hits"), STAT_RAC_NumCacheHits, STATGROUP_RAC, );
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("RAC Num Retrieve fails"), STAT_RAC_NumFails, STATGROUP_RAC, );
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("RAC Num Gets"), STAT_RAC_NumGets, STATGROUP_RAC, );
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("RAC Num Puts"), STAT_RAC_NumPuts, STATGROUP_RAC, );

/** Forward declarations. */
class FRuntimeAssetCacheBuilderInterface;

/**
 * Worker to retrieve entry from cache or build it in case of cache miss.
 */
class FRuntimeAssetCacheAsyncWorker : public FNonAbandonableTask
{
public:
	/** Constructor */
	FRuntimeAssetCacheAsyncWorker(FRuntimeAssetCacheBuilderInterface* InCacheBuilder, TMap<FName, FRuntimeAssetCacheBucket*>* InBuckets);

	/** Async task interface implementation. */
	void DoWork();
	TStatId GetStatId();
	/** End of async task interface implementation. */

	/** Gets serialized cache data. */
	TArray<uint8>& GetData()
	{
		return Data;
	}

	/**
	 * Checks if entry was successfully retrieved from cache.
	 * @return True on success, false otherwise. Success means either cache hit or miss, as long as we got valid data.
	 */
	bool RetrievedEntry() const
	{
		return bEntryRetrieved;
	}
private:
	/**
	* Static function to make sure a cache key contains only legal characters by using an escape.
	* @param CacheKey Cache key to sanitize
	* @return Sanitized cache key
	**/
	static FString SanitizeCacheKey(const TCHAR* CacheKey);

	/**
	* Static function to build a cache key out of the plugin name, versions and plugin specific info
	* @param PluginName Name of the runtime asset data type
	* @param VersionString Version string of the runtime asset data
	* @param PluginSpecificCacheKeySuffix Whatever is needed to uniquely identify the specific cache entry.
	* @return Assembled cache key
	**/
	static FString BuildCacheKey(const TCHAR* VersionString, const TCHAR* PluginSpecificCacheKeySuffix);

	/**
	* Static function to build a cache key out of the CacheBuilder.
	* @param CacheBuilder Builder to create key from.
	* @return Assembled cache key
	**/
	static FString BuildCacheKey(FRuntimeAssetCacheBuilderInterface* CacheBuilder);

	/**
	* Removes entries from cache until given number of bytes are freed (or more).
	* @param Bucket Bucket to clean up.
	* @param SizeOfSpaceToFreeInBytes Number of bytes to free (or more).
	*/
	void FreeCacheSpace(FName Bucket, int32 SizeOfSpaceToFreeInBytes);

	/** Cache builder to create cache entry in case of cache miss. */
	FRuntimeAssetCacheBuilderInterface* CacheBuilder;
	
	/** Data to return to caller. */
	TArray<uint8> Data;

	/** Reference to map of bucket names to their descriptions. */
	TMap<FName, FRuntimeAssetCacheBucket*>* Buckets;

	/**
	 * True if successfully retrieved a cache entry. Can be false only when CacheBuilder
	 * returns false or no CacheBuilder was provided.
	 */
	bool bEntryRetrieved;
};