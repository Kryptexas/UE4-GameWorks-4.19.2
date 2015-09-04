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

public: 
	FBlueprintNativeCodeGenManifest(const FString TargetPath = TEXT(""));
	FBlueprintNativeCodeGenManifest(const FNativeCodeGenCommandlineParams& CommandlineParams);

	FORCEINLINE const FString& GetTargetPath() const { return ModulePath; }

	/**  */
	bool Save() const;

	/**  */
	FConvertedAssetRecord& CreateConversionRecord(const FAssetData& AssetInfo);

private:
	UPROPERTY()
	FString ModulePath;

	UPROPERTY()
	TArray<FString> ModuleDependencies;

	UPROPERTY()
	TArray<FConvertedAssetRecord> ConvertedAssets; 
	/**  */
	TMap<FName, int32> RecordLookupTable;
};
