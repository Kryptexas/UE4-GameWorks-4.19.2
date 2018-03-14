// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "DataValidationManager.h"

#include "ModuleManager.h"
#include "Developer/MessageLog/Public/MessageLogModule.h"
#include "Logging/MessageLog.h"
#include "ScopedSlowTask.h"

#include "CoreGlobals.h"

#define LOCTEXT_NAMESPACE "DataValidationManager"

UDataValidationManager* GDataValidationManager = nullptr;

/**
 * UDataValidationManager
 */

UDataValidationManager::UDataValidationManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DataValidationManagerClassName = FSoftClassPath(TEXT("/Script/DataValidation.DataValidationManager"));
}

UDataValidationManager* UDataValidationManager::Get()
{
	if (GDataValidationManager == nullptr)
	{
		FSoftClassPath DataValidationManagerClassName = (UDataValidationManager::StaticClass()->GetDefaultObject<UDataValidationManager>())->DataValidationManagerClassName;

		UClass* SingletonClass = DataValidationManagerClassName.TryLoadClass<UObject>();
		checkf(SingletonClass != nullptr, TEXT("Data validation config value DataValidationManagerClassName is not a valid class name."));

		GDataValidationManager = NewObject<UDataValidationManager>(GetTransientPackage(), SingletonClass, NAME_None);
		checkf(GDataValidationManager != nullptr, TEXT("Data validation config value DataValidationManagerClassName is not a subclass of UDataValidationManager."))

		GDataValidationManager->AddToRoot();
		GDataValidationManager->Initialize();
	}

	return GDataValidationManager;
}

void UDataValidationManager::Initialize()
{
	FMessageLogInitializationOptions InitOptions;
	InitOptions.bShowFilters = true;

	FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	MessageLogModule.RegisterLogListing("DataValidation", LOCTEXT("DataValidation", "Data Validation"), InitOptions);
}

UDataValidationManager::~UDataValidationManager()
{
}

EDataValidationResult UDataValidationManager::IsObjectValid(UObject* InObject, TArray<FText>& ValidationErrors) const
{
	if (ensure(InObject))
	{
		return InObject->IsDataValid(ValidationErrors);
	}

	return EDataValidationResult::NotValidated;
}

EDataValidationResult UDataValidationManager::IsAssetValid(FAssetData& AssetData, TArray<FText>& ValidationErrors) const
{
	if (AssetData.IsValid())
	{
		UObject* Obj = AssetData.GetAsset();
		if (Obj)
		{
			return IsObjectValid(Obj, ValidationErrors);
		}
	}

	return EDataValidationResult::Invalid;
}

void UDataValidationManager::ValidateAssets(TArray<FAssetData> AssetDataList, bool bSkipExcludedDirectories) const
{
	FScopedSlowTask SlowTask(1.0f, LOCTEXT("ValidatingDataTask", "Validating Data..."));
	SlowTask.Visibility = ESlowTaskVisibility::ForceVisible;
	SlowTask.MakeDialog();

	FMessageLog DataValidationLog("DataValidation");

	int32 NumAdded = 0;

	int32 NumFilesChecked = 0;
	int32 NumValidFiles = 0;
	int32 NumInvalidFiles = 0;
	int32 NumFilesSkipped = 0;
	int32 NumFilesUnableToValidate = 0;

	int32 NumFilesToValidate = AssetDataList.Num();

	// Now add to map or update as needed
	for (FAssetData& Data : AssetDataList)
	{
		SlowTask.EnterProgressFrame(1.0f / NumFilesToValidate, FText::Format(LOCTEXT("ValidatingFilename", "Validating {0}"), FText::FromString(Data.GetFullName())));

		// Check exclusion path
		if (bSkipExcludedDirectories && IsPathExcludedFromValidation(Data.PackageName.ToString()))
		{
			++NumFilesSkipped;
			continue;
		}

		TArray<FText> ValidationErrors;
		EDataValidationResult Result = IsAssetValid(Data, ValidationErrors);
		++NumFilesChecked;

		for (const FText& ErrorMsg : ValidationErrors)
		{
			FMessageLog("DataValidation").Error()->AddToken(FTextToken::Create(ErrorMsg));
		}

		if (Result == EDataValidationResult::Valid)
		{
			++NumValidFiles;
		}
		else
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("Filename"), FText::FromName(Data.PackageName));
			if (Result == EDataValidationResult::Invalid)
			{
				FMessageLog("DataValidation").Error()->AddToken(FTextToken::Create(FText::Format(LOCTEXT("InvalidDataResult", "{Filename} contains invalid data."), Arguments)));
				++NumInvalidFiles;
			}
			else if (Result == EDataValidationResult::NotValidated)
			{
				FMessageLog("DataValidation").Info()->AddToken(FTextToken::Create(FText::Format(LOCTEXT("NotValidatedDataResult", "There is no data validation for {Filename}."), Arguments)));
				++NumFilesUnableToValidate;
			}
		}
	}

	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Result"), NumInvalidFiles > 0 ? LOCTEXT("Failed", "FAILED") : LOCTEXT("Succeeded", "SUCCEEDED"));
		Arguments.Add(TEXT("NumChecked"), NumFilesChecked);
		Arguments.Add(TEXT("NumValid"), NumValidFiles);
		Arguments.Add(TEXT("NumInvalid"), NumInvalidFiles);
		Arguments.Add(TEXT("NumSkipped"), NumFilesSkipped);
		Arguments.Add(TEXT("NumUnableToValidate"), NumFilesUnableToValidate);

		TSharedRef<FTokenizedMessage> ValidationLog = NumInvalidFiles > 0 ? FMessageLog("DataValidation").Error() : FMessageLog("DataValidation").Info();

		ValidationLog->AddToken(FTextToken::Create(FText::Format(LOCTEXT("SuccessOrFailure", "Data validation {Result}."), Arguments)));
		ValidationLog->AddToken(FTextToken::Create(FText::Format(LOCTEXT("ResultsSummary", "Files Checked: {NumChecked}, Passed: {NumValid}, Failed: {NumInvalid}, Skipped: {NumSkipped}, Unable to validate: {NumUnableToValidate}"), Arguments)));
	}

	DataValidationLog.Open(EMessageSeverity::Info, true);
}

bool UDataValidationManager::IsPathExcludedFromValidation(const FString& Path) const
{
	for (const FDirectoryPath& ExcludedPath : ExcludedDirectories)
	{
		if (Path.Contains(ExcludedPath.Path))
		{
			return true;
		}
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
