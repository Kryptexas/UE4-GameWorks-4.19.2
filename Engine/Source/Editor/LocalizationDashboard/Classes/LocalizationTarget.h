// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"
#include "LocalizationTarget.generated.h"

UENUM()
enum class ELocalizationTargetConflictStatus : uint8
{
	Unknown,
	ConflictsPresent,
	Clear
};

USTRUCT()
struct FGatherTextFromTextFilesConfiguration
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(config, EditAnywhere, Category = "Filter")
	TArray<FString> SearchDirectories;

	UPROPERTY(config, EditAnywhere, Category = "Filter")
	TArray<FString> ExcludePathWildcards;

	UPROPERTY(config, EditAnywhere, Category = "Filter")
	TArray<FString> FileExtensions;
};

USTRUCT()
struct FGatherTextFromPackagesConfiguration
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(config, EditAnywhere, Category = "Filter")
	TArray<FString> IncludePathWildcards;

	UPROPERTY(config, EditAnywhere, Category = "Filter")
	TArray<FString> ExcludePathWildcards;

	UPROPERTY(config, EditAnywhere, Category = "Filter")
	TArray<FString> FileExtensions;
};

USTRUCT()
struct FMetaDataKeyGatherSpecification
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(config, EditAnywhere, Category = "Input")
	FString MetaDataKey;

	UPROPERTY(config, EditAnywhere, Category = "Output")
	FString TextNamespace;

	UPROPERTY(config, EditAnywhere, Category = "Output")
	FString TextKeyPattern;
};

USTRUCT()
struct FGatherTextFromMetaDataConfiguration
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(config, EditAnywhere, Category = "Filter")
	TArray<FString> IncludePathWildcards;

	UPROPERTY(config, EditAnywhere, Category = "Filter")
	TArray<FString> ExcludePathWildcards;

	UPROPERTY(config, EditAnywhere, Category = "MetaData")
	TArray<FMetaDataKeyGatherSpecification> KeySpecifications;
};

USTRUCT()
struct FCultureStatistics
{
	GENERATED_USTRUCT_BODY()

	FCultureStatistics()
		: WordCount(0)
	{
	}

	UPROPERTY(config, EditAnywhere, Category = "Culture")
	FString CultureName;

	UPROPERTY(Transient, EditAnywhere, Category = "Statistics")
	uint32 WordCount;
};

USTRUCT()
struct FLocalizationTargetSettings
{
	GENERATED_USTRUCT_BODY()

	FLocalizationTargetSettings();

	/* Unique name for the target within the context of the project. */
	UPROPERTY(config, EditAnywhere, Category = "Target")
	FString Name;

	UPROPERTY(Transient, EditAnywhere, Category = "Target")
	ELocalizationTargetConflictStatus ConflictStatus;

	UPROPERTY(config, EditAnywhere, Category = "Gather Configuration")
	TArray<FString> TargetDependencies;

	UPROPERTY(config, EditAnywhere, Category = "Gather Configuration", AdvancedDisplay, meta=(FilePathFilter="manifest"))
	TArray<FFilePath> AdditionalManifestDependencies;

	UPROPERTY(config, EditAnywhere, Category = "Gather Configuration", AdvancedDisplay)
	TArray<FString> RequiredModuleNames;

	UPROPERTY(config, EditAnywhere, Category = "Gather Configuration")
	FGatherTextFromTextFilesConfiguration GatherFromTextFiles;

	UPROPERTY(config, EditAnywhere, Category = "Gather Configuration")
	FGatherTextFromPackagesConfiguration GatherFromPackages;

	UPROPERTY(config, EditAnywhere, Category = "Gather Configuration")
	FGatherTextFromMetaDataConfiguration GatherFromMetaData;

	UPROPERTY(config, EditAnywhere, Category = "Cultures")
	FCultureStatistics NativeCultureStatistics;

	UPROPERTY(config, EditAnywhere, Category = "Cultures")
	TArray<FCultureStatistics> SupportedCulturesStatistics;
};

UCLASS(Within=LocalizationTargetSet)
class ULocalizationTarget : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Target")
	FLocalizationTargetSettings Settings;

public:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	bool IsMemberOfEngineTargetSet() const;
	bool UpdateWordCountsFromCSV();
	void UpdateStatusFromConflictReport();
	bool RenameTargetAndFiles(const FString& NewName);
	bool DeleteFiles(const FString* const Culture = nullptr) const;
};

UCLASS(Config=Localization, perObjectConfig, defaultconfig)
class ULocalizationTargetSet : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Targets")
	TArray<ULocalizationTarget*> TargetObjects;

	static FName EngineTargetSetName;
	static FName ProjectTargetSetName;

private:
	UPROPERTY(config)
	TArray<FLocalizationTargetSettings> Targets;

public:
#if WITH_EDITOR
	virtual void PostInitProperties() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};