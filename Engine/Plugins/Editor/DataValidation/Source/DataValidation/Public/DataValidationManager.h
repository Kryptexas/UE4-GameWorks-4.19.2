// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AssetData.h"
#include "Engine/EngineTypes.h"
#include "DataValidationManager.generated.h"

class FLogWindowManager;
class SLogWindow;
class SWindow;
class UUnitTest;

/**
 * Manages centralized execution and tracking of data validation, as well as handling console commands,
 * and some misc tasks like local log hooking
 */

UCLASS(config=Editor)
class DATAVALIDATION_API UDataValidationManager : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	/**
	 * Static getter for the data validation manager
	 *
	 * @return	Returns the data validation manager
	 */
	static UDataValidationManager* Get();

	/**
	 * Initialize the data validation manager
	 */
	virtual void Initialize();

	/**
	 * Destructor for handling removal of log registration
	 */
	virtual ~UDataValidationManager() override;

	/**
	 * @return Returns Valid if the object contains valid data; returns Invalid if the object contains invalid data; returns NotValidated if no validations was performed on the object
	 */
	virtual EDataValidationResult IsObjectValid(UObject* InObject, TArray<FText>& ValidationErrors) const;

	/**
	 * @return Returns Valid if the object pointed to by AssetData contains valid data; returns Invalid if the object contains invalid data or does not exist; returns NotValidated if no validations was performed on the object
	 */
	virtual EDataValidationResult IsAssetValid(FAssetData& AssetData, TArray<FText>& ValidationErrors) const;

	virtual void ValidateAssets(TArray<FAssetData> AssetDataList, bool bSkipExcludedDirectories = true) const;

protected:
	/** 
	 * @return Returns true if the current Path should be skipped for validation. Returns false otherwise.
	 */
	virtual bool IsPathExcludedFromValidation(const FString& Path) const;

	/**
	 * Directories to ignore for data validation. Useful for test assets
	 */
	UPROPERTY(config)
	TArray<FDirectoryPath> ExcludedDirectories;

private:

	/** The class to instantiate as the manager object. Defaults to this class but can be overridden */
	UPROPERTY(config)
	FSoftClassPath DataValidationManagerClassName;
};
