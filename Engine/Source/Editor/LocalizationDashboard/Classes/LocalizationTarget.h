// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"
#include "LocalizationTarget.generated.h"

UENUM()
enum class ELocalizationTargetStatus : uint8
{
	Unknown,
	ConflictsPresent,
	Clear
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
	ELocalizationTargetStatus Status;

	UPROPERTY(config, EditAnywhere, Category = "Target")
	TArray<FString> TargetDependencies;

	UPROPERTY(config, EditAnywhere, Category = "Target", AdvancedDisplay, meta=(FilePathFilter="manifest"))
	TArray<FFilePath> AdditionalManifestDependencies;

	UPROPERTY(config, EditAnywhere, Category = "Cultures")
	FCultureStatistics NativeCultureStatistics;

	UPROPERTY(config, EditAnywhere, Category = "Cultures")
	TArray<FCultureStatistics> SupportedCulturesStatistics;

	bool UpdateWordCountsFromCSV();
	void UpdateStatusFromConflictReport();
	bool Rename(const FString& NewName);
	bool DeleteFiles(const FString* const Culture = nullptr) const;
};

UCLASS(Within=ProjectLocalizationSettings)
class ULocalizationTarget : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Target")
	FLocalizationTargetSettings Settings;

public:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};