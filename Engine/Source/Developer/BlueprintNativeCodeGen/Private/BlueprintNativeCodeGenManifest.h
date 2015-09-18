// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintNativeCodeGenManifest.generated.h"

// Forward declarations
struct FNativeCodeGenCommandlineParams;
class  FAssetData;

/*******************************************************************************
 * FCodeGenAssetRecord
 ******************************************************************************/

USTRUCT()
struct FConvertedAssetRecord
{
	GENERATED_USTRUCT_BODY()

public:
	TWeakObjectPtr<UObject> AssetPtr;

	UPROPERTY()
	UClass* AssetType;

	UPROPERTY()
	FString AssetPath;

	UPROPERTY()
	FString GeneratedHeaderPath;

	UPROPERTY()
	FString GeneratedCppPath;
};

/*******************************************************************************
 * FBlueprintNativeCodeGenManifest
 ******************************************************************************/
 
USTRUCT()
struct FBlueprintNativeCodeGenManifest
{
	GENERATED_USTRUCT_BODY()

	static FString GetDefaultFilename();

public: 
	FBlueprintNativeCodeGenManifest(const FString TargetPath = TEXT(""));
	FBlueprintNativeCodeGenManifest(const FNativeCodeGenCommandlineParams& CommandlineParams);

	/**
	 * 
	 * 
	 * @return 
	 */
	TArray<FString> GetTargetPaths() const;

	/**
	 * 
	 * 
	 * @param  AssetInfo    
	 * @return 
	 */
	FConvertedAssetRecord& CreateConversionRecord(const FAssetData& AssetInfo);

	/**
	 * 
	 * 
	 * @param  AssetPath    
	 * @return 
	 */
	FConvertedAssetRecord* FindConversionRecord(const FString& AssetPath, bool bSlowSearch = false);

	/**
	 * 
	 */
	bool Save(bool bSort = true) const;

private:
	/**
	 * 
	 */
	FString GetModuleDir() const;

	/**
	 * 
	 */
	void Clear();

	/**
	 *
	 */
	void MapConvertedAssets();
	
	/**  */
	FString ManifestPath;

	/** Relative to the project's directory */
	UPROPERTY()
	FString ModulePath;

	/** Mutable so Save() can sort the array when requested */
	UPROPERTY()
	mutable TArray<FConvertedAssetRecord> ConvertedAssets;

	/**  */
	TMap<FString, int32> RecordMap;
};
