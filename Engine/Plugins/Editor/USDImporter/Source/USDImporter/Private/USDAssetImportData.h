// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "EditorFramework/AssetImportData.h"
#include "USDAssetImportData.generated.h"

/**
* Import data and options used when importing an asset from USD
*/
UCLASS(config = EditorPerProjectUserSettings, AutoExpandCategories = (Options), MinimalAPI)
class UUSDAssetImportData : public UAssetImportData
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	class UUSDImportOptions* ImportOptions;
};