// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AssetImportData.generated.h"

/*
 * A class to hold import options and data for assets. This class has many children in editor code.
 */
UCLASS(MinimalAPI)
class UAssetImportData : public UObject
{
	GENERATED_UCLASS_BODY()

	/** Path to the resource used to construct this static mesh. Relative to the object's package, BaseDir() or absolute */
	UPROPERTY(EditAnywhere, Category=ImportSettings)
	FString SourceFilePath;

	/** Date/Time-stamp of the file from the last import */
	UPROPERTY(VisibleAnywhere, Category=ImportSettings)
	FString SourceFileTimestamp;
};



