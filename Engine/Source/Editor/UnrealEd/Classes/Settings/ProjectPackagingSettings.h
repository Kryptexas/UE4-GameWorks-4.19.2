// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ProjectPackagingSettings.h: Declares the UProjectPackagingSettings class.
=============================================================================*/

#pragma once

#include "ProjectPackagingSettings.generated.h"


/**
 * Enumerates the available build configurations for project packaging.
 */
UENUM()
enum EProjectPackagingBuildConfigurations
{
	/**
	 * Debug configuration.
	 */
	PPBC_DebugGame UMETA(DisplayName="DebugGame"),

	/**
	 * Development configuration.
	 */
	PPBC_Development UMETA(DisplayName="Development"),

	/**
	 * Shipping configuration.
	 */
	PPBC_Shipping UMETA(DisplayName="Shipping")
};


/**
 * Structure for directory paths that are displayed in the UI.
 */
USTRUCT()
struct FDirectoryPath
{
	GENERATED_USTRUCT_BODY()

	/**
	 * The path to the directory.
	 */
	UPROPERTY(EditAnywhere, Category=Path)
	FString Path;
};


/**
 * Implements the Editor's user settings.
 */
UCLASS(config=Game)
class UNREALED_API UProjectPackagingSettings
	: public UObject
{
	GENERATED_UCLASS_BODY()

	// Begin UObject Interface

	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent ) OVERRIDE;
	
	// End UObject Interface

public:

	/**
	 * The build configuration for which the project is packaged.
	 */
	UPROPERTY(config, EditAnywhere, Category=Project)
	TEnumAsByte<EProjectPackagingBuildConfigurations> BuildConfiguration;

	/**
	 * The directory to which the packaged project will be copied.
	 */
	UPROPERTY(config, EditAnywhere, Category=Project)
	FDirectoryPath StagingDirectory;

	/**
	 * If enabled, a full rebuild will be enforced each time the project is being packaged.
	 * If disabled, only modified files will be built, which can improve iteration time.
	 * Unless you iterate on packaging, we recommend full rebuilds when packaging.
	 */
	UPROPERTY(config, EditAnywhere, Category=Project, AdvancedDisplay)
	bool FullRebuild;

	/**
	 * If enabled, a distribution build will be created
	 * If disabled, a development build will be created
	 * Distribution builds are for publishing to the App Store
	 */
	UPROPERTY(config, EditAnywhere, Category=Project, AdvancedDisplay)
	bool ForDistribution;

	/**
	 * All assets in these directories will always be cooked.
	 */
	UPROPERTY(EditAnywhere, config, Category=Cooking)
	TArray<FDirectoryPath> DirectoriesToAlwaysCook;

	/**
	 * If enabled, all content will be put into a single .pak file instead of many individual files (default = enabled).
	 */
	UPROPERTY(config, EditAnywhere, Category=Packaging)
	bool UsePakFile;

	/*
	 * Updates visibility metadata for values of the EProjectPackagingBuildConfigurations, as appropriate for the current project.
	 */
	static void UpdateBuildConfigurationVisibility();
};
