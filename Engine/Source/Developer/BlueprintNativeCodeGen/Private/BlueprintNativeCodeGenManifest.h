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

	FString GetTargetPath() const;

	/**  */
	bool Save() const;

	/**  */
	FConvertedAssetRecord& CreateConversionRecord(const FAssetData& AssetInfo);

private:
	void Clear();

	/**  */
	FString ManifestPath;

	/** Relative to the project's directory */
	UPROPERTY()
	FString ModulePath;

	UPROPERTY()
	TArray<FString> ModuleDependencies;

	UPROPERTY()
	TArray<FConvertedAssetRecord> ConvertedAssets;
};
