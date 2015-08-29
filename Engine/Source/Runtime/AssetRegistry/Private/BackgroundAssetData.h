// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SharedMapView.h"

/** Interface to the gathered asset data */
class IGatheredAssetData
{
public:
	virtual ~IGatheredAssetData() {}

	/** Creates an FAssetData object for this object */
	virtual FAssetData ToAssetData() const = 0;

	/** Test to see if this asset data is within the given search path */
	virtual bool IsWithinSearchPath(const FString& InSearchPath) const = 0;
};

/** A class to hold important information about an assets found by the Asset Registry. Intended for use by the background gathering thread to avoid creation of FNames */
class FBackgroundAssetData : public IGatheredAssetData
{
public:
	/** Constructor */
	FBackgroundAssetData(FString InPackageName, FString InPackagePath, FString InGroupNames, FString InAssetName, FString InAssetClass, TMap<FString, FString> InTags, TArray<int32> InChunkIDs);

	/** IGatheredAssetData interface */
	virtual FAssetData ToAssetData() const override;
	virtual bool IsWithinSearchPath(const FString& InSearchPath) const override;
	
private:
	/** The object path for the asset in the form 'Package.GroupNames.AssetName' */
	FString ObjectPath;
	/** The name of the package in which the asset is found */
	FString PackageName;
	/** The path to the package in which the asset is found */
	FString PackagePath;
	/** The '.' delimited list of group names in which the asset is found. NAME_None if there were no groups */
	FString GroupNames;
	/** The name of the asset without the package or groups */
	FString AssetName;
	/** The name of the asset's class */
	FString AssetClass;
	/** The map of values for properties that were marked AssetRegistrySearchable */
	TSharedMapView<FString, FString> TagsAndValues;
	/** The IDs of the chunks this asset is located in for streaming install.  Empty if not assigned to a chunk */
	TArray<int32> ChunkIDs;
};

/** A class to wrap up an existing FAssetData to avoid redundant FName -> FString -> FName conversion */
class FAssetDataWrapper : public IGatheredAssetData
{
public:
	/** Constructor */
	FAssetDataWrapper(FAssetData InAssetData);

	/** IGatheredAssetData interface */
	virtual FAssetData ToAssetData() const override;
	virtual bool IsWithinSearchPath(const FString& InSearchPath) const override;

private:
	/** The asset data we're wrapping up */
	FAssetData WrappedAssetData;
	/** The value of WrappedAssetData.PackagePath as an FString for use by IsWithinSearchPath */
	FString PackagePathStr;
};
