// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AssetData.h"
#include "BlueprintNativeCodeGenManifest.h"
#include "BlueprintNativeCodeGenCoordinator.generated.h"

// Forward declarations
struct FNativeCodeGenCommandlineParams;

UCLASS(config=Editor)
class UBlueprintNativeCodeGenConfig : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(globalconfig)
	TArray<FString> PackagesToAlwaysConvert;
};

/**  */
struct FBlueprintNativeCodeGenCoordinator
{
public:
	FBlueprintNativeCodeGenCoordinator(const FNativeCodeGenCommandlineParams& CommandlineParams);

	bool IsTargetedForConversion(const FString AssetPath);
	bool IsTargetedForConversion(const UBlueprint* Blueprint);
	bool IsTargetedForConversion(const UClass* Class);
	bool IsTargetedForConversion(const UEnum* Enum);
	bool IsTargetedForConversion(const UStruct* Struct);

public:
	TArray<FAssetData> ConversionQueue;
	FBlueprintNativeCodeGenManifest Manifest;
};

