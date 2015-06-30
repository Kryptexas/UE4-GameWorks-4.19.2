// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "RuntimeAssetCachePrivatePCH.h"
#include "RuntimeAssetCacheBuilders.h"

void URuntimeAssetCacheBuilder_ObjectBase::SaveNewAssetToCache(UObject* NewAsset)
{
	SetAsset(NewAsset);
	GetFromCacheAsync(OnAssetCacheComplete);
}

void URuntimeAssetCacheBuilder_ObjectBase::SetAsset(UObject* NewAsset)
{
	Asset = NewAsset;
	OnSetAsset(Asset);
}

FVoidPtrParam URuntimeAssetCacheBuilder_ObjectBase::Build()
{
	// There was no cached asset, so this is expecting us to return the data that needs to be saved to disk
	// If we have no asset created yet, just return null. That will trigger the async creation of the asset.
	// If we do have an asset, serialize it here into a good format and return a pointer to that memory buffer.
	if (Asset)
	{
		int64 DataSize = GetSerializedDataSizeEstimate();
		void* Result = new uint8[DataSize];
		FBufferWriter Ar(Result, DataSize, false);
		Ar.ArIsPersistent = true;
		SerializeAsset(Ar);

		return FVoidPtrParam(Result, Ar.Tell());
	}

	// null
	return FVoidPtrParam::NullPtr();
}

void URuntimeAssetCacheBuilder_ObjectBase::GetFromCacheAsync(const FOnAssetCacheComplete& OnComplete)
{
	OnAssetCacheComplete = OnComplete;
	GetFromCacheAsyncCompleteDelegate.BindDynamic(this, &URuntimeAssetCacheBuilder_ObjectBase::GetFromCacheAsyncComplete);
	GetRuntimeAssetCache().GetAsynchronous(this, GetFromCacheAsyncCompleteDelegate);
}

void URuntimeAssetCacheBuilder_ObjectBase::GetFromCacheAsyncComplete(int32 Handle, FVoidPtrParam DataPtr)
{
	if (DataPtr.Data != nullptr)
	{
		// Success! Finished loading or saving data from cache
		// If saving, then we already have the right data and we can just report success
		if (Asset == nullptr)
		{
			// If loading, we now need to serialize the data into a usable format

			// Make sure Asset is set up to be loaded into
			OnAssetPreLoad();

			FBufferReader Ar(DataPtr.Data, DataPtr.DataSize, false);
			SerializeAsset(Ar);

			// Perform any specific init functions after load
			OnAssetPostLoad();
		}

		// Free the buffer memory on both save and load
		// On save the buffer gets created in Build()
		// On load the buffer gets created in FRuntimeAssetCacheBackend::GetCachedData()
		FMemory::Free(DataPtr.Data);

		// Success!
		OnAssetCacheComplete.ExecuteIfBound(this, true);
		CacheHandle = 0;
	}
	else
	{
		// Data not on disk. Kick off the creation process.
		// Once complete, call GetFromCacheAsync() again and it will loop back to this function, but should succeed.
		// But, prevent recursion the second time by checking if the CacheHandle is already set.
		if (CacheHandle == 0)
		{
			CacheHandle = Handle;
			OnAssetCacheMiss();
		}
		else
		{
			// Failed
			OnAssetCacheComplete.ExecuteIfBound(this, false);
		}
	}
}
