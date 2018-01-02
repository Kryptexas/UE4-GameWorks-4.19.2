// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "DataValidationCommandlet.h"
#include "DataValidationManager.h"

#include "IAssetRegistry.h"
#include "AssetRegistryHelpers.h"
#include "AssetRegistryModule.h"


DEFINE_LOG_CATEGORY_STATIC(LogDataValidation, Warning, All);

// Commandlet for validating data
int32 UDataValidationCommandlet::Main(const FString& FullCommandLine)
{
	UE_LOG(LogDataValidation, Log, TEXT("--------------------------------------------------------------------------------------------"));
	UE_LOG(LogDataValidation, Log, TEXT("Running DataValidation Commandlet"));
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> Params;
	ParseCommandLine(*FullCommandLine, Tokens, Switches, Params);

	// validate data
	if (!ValidateData())
	{
		UE_LOG(LogDataValidation, Warning, TEXT("Errors occurred while validating data"));
		return 2; // return something other than 1 for error since the engine will return 1 if any other system (possibly unrelated) logged errors during execution.
	}

	UE_LOG(LogDataValidation, Log, TEXT("Successfully finished running DataValidation Commandlet"));
	UE_LOG(LogDataValidation, Log, TEXT("--------------------------------------------------------------------------------------------"));
	return 0;
}

//static
bool UDataValidationCommandlet::ValidateData()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAllAssets(AssetDataList);

	UDataValidationManager* DataValidationManager = UDataValidationManager::Get();
	check(DataValidationManager);

	DataValidationManager->ValidateAssets(AssetDataList);

	return true;
}
