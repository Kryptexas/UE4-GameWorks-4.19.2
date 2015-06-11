// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
* Interface for runtime asset cache cache builders.
* This API will be called concurrently.
**/
class FRuntimeAssetCacheBuilderInterface
{
public:
	/** Virtual destructor. */
	virtual ~FRuntimeAssetCacheBuilderInterface()
	{ }

	/**
	* Get the builder type name. This is used to categorize cached data to buckets.
	* @return Type name of builder.
	**/
	virtual const TCHAR* GetTypeName() const = 0;

	/**
	* Get the name of cache builder, this is used as part of the cache key.
	* @return Name of cache builder
	**/
	virtual const TCHAR* GetBuilderName() const = 0;

	/**
	* Returns the builder specific part of the cache key. This must be a alphanumeric+underscore
	* @return Version number of the builder.
	**/
	virtual FString GetAssetUniqueName() const = 0;

	/**
	* Does the work of creating serialized cache entry.
	* @param OutData Array of bytes to fill in with the result data
	* @return true if successful, in the event of failure the cache is not updated and failure is propagated to the original caller.
	**/
	virtual bool Build(TArray<uint8>& OutData) = 0;

	/**
	 * Gets asset version to rebuild cache if cached asset is too old.
	 * @return Asset version.
	 */
	virtual int32 GetAssetVersion() = 0;
};
