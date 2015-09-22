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
	FConvertedAssetRecord() {}
	FConvertedAssetRecord(const FAssetData& AssetInfo, const FString& TargetModulePath);

	bool IsValid();

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
	 * A query that retrieves a list of file/directory paths. These paths are 
	 * the destination targets for files and directories that should/will be 
	 * generated as part of the conversion process.
	 * 
	 * @return An array of file/directory paths that serve as destinations for the conversion process.
	 */
	TArray<FString> GetTargetPaths() const;

	/**  */
	FString GetBuildFilePath() const;

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

	/**  */
	bool LogDependencies(const FAssetData& AssetInfo);

	/**  */
	const TArray<UPackage*>& GetModuleDependencies() const { return ModuleDependencies; }

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

	UPROPERTY()
	FString ModuleName;

	/** Relative to the project's directory */
	UPROPERTY()
	FString ModulePath;

	UPROPERTY()
	TArray<UPackage*> ModuleDependencies;

	/** Mutable so Save() can sort the array when requested */
	UPROPERTY()
	mutable TArray<FConvertedAssetRecord> ConvertedAssets;

	/**  */
	TMap<FString, int32> RecordMap;
};
