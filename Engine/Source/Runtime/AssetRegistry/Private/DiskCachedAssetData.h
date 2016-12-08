// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetData.h"
#include "PackageDependencyData.h"

class FDiskCachedAssetData
{
public:
	FDateTime Timestamp;
	TArray<FAssetData> AssetDataList;
	FPackageDependencyData DependencyData;

	FDiskCachedAssetData()
	{}

	FDiskCachedAssetData(const FDateTime& InTimestamp)
		: Timestamp(InTimestamp)
	{}

	/** Operator for serialization */
	friend FArchive& operator<<(FArchive& Ar, FDiskCachedAssetData& DiskCachedAssetData)
	{
		Ar << DiskCachedAssetData.Timestamp;
		if (Ar.IsError())
		{
			return Ar;
		}

		Ar << DiskCachedAssetData.AssetDataList;
		if (Ar.IsError())
		{
			return Ar;
		}

		Ar << DiskCachedAssetData.DependencyData;
		
		return Ar;
	}
};
