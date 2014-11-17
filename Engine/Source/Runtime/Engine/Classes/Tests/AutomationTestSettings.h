// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AutomationTestSettings.h: Declares the UAutomationTestSettings class.
=============================================================================*/

#pragma once

#include "AutomationTestSettings.generated.h"


/**
 * Structure for asset opening test
 */
USTRUCT()
struct FOpenTestAsset
{
	GENERATED_USTRUCT_BODY()

	/**
	 * Asset reference
	 */
	UPROPERTY(EditAnywhere, Category=Automation)
	FFilePath	AssetToOpen;

	/** Perform only when attend **/
	UPROPERTY(EditAnywhere, Category=Automation)
	bool		bSkipTestWhenUnAttended;
};

/**
 * Structure for defining an external tool
 */
USTRUCT()
struct FExternalToolDefinition
{
	GENERATED_USTRUCT_BODY()

	/* The name of the tool / test. */
	UPROPERTY(config, EditAnywhere, Category=ExternalTools)
	FString ToolName;

	/* The executable to run. */
	UPROPERTY(config, EditAnywhere, Category=ExternalTools, meta=(FilePathFilter = "*"))
	FFilePath ExecutablePath;

	/* The command line options to pass to the executable. */
	UPROPERTY(config, EditAnywhere, Category=ExternalTools)
	FString CommandLineOptions;

	/* The working directory for the new process. */
	UPROPERTY(config, EditAnywhere, Category=ExternalTools, meta=(RelativePath))
	FDirectoryPath WorkingDirectory;

	/* If set, look for scripts with this extension. */
	UPROPERTY(config, EditAnywhere, Category=ExternalTools)
	FString ScriptExtension;

	/* If the ScriptExtension is set, look here for the script files. */
	UPROPERTY(config, EditAnywhere, Category=ExternalTools, meta=(RelativePath))
	FDirectoryPath ScriptDirectory;
};

/**
 * Holds UProperty names and values to customize factory settings
 */
USTRUCT()
struct FImportFactorySettingValues
{
	GENERATED_USTRUCT_BODY()

	/* The name of the UProperty to change */
	UPROPERTY(config, EditAnywhere, Category = Automation, meta = (ToolTip = "Name of the property to change.  Nested settings can be modified using \"Outer.Property\""))
	FString SettingName;

	/* The value to apply to the UProperty */
	UPROPERTY(config, EditAnywhere, Category = Automation, meta = (ToolTip = "Value to import for the specified property."))
	FString Value;
};


/**
 * Holds settings for the asset import / export automation test
 */
USTRUCT()
struct FEditorImportExportTestDefinition
{
	GENERATED_USTRUCT_BODY()

	/* The file to import */
	UPROPERTY(config, EditAnywhere, Category = Automation, meta = (FilePathFilter = "*"))
	FFilePath ImportFilePath;

	/* The file extension to use when exporting */
	UPROPERTY(config, EditAnywhere, Category = Automation, meta = (ToolTip = "The file extension to use when exporting this asset.  Used to find a supporting exporter"))
	FString ExportFileExtension;

	/* If true, the export step will be skipped */
	UPROPERTY(config, EditAnywhere, Category = Automation)
	bool bSkipExport;

	/* Settings for the import factory */
	UPROPERTY(config, EditAnywhere, Category=Automation)
	TArray<FImportFactorySettingValues> FactorySettings;
};

/**
 * Implements the Editor's user settings.
 */
UCLASS(config=Engine)
class ENGINE_API UAutomationTestSettings : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	/**
	 * The directory to which the packaged project will be copied.
	 */
	UPROPERTY(config, EditAnywhere, Category=Automation, meta=(FilePathFilter = "umap"))
	FFilePath AutomationTestmap;

	/**
	 * Asset to test for open in automation process
	 */
	UPROPERTY(EditAnywhere, config, Category=Automation)
	TArray<FOpenTestAsset> TestAssetsToOpen;

	/**
	 * Modules to load that have engine tests
	 */
	UPROPERTY(EditAnywhere, config, Category=Automation)
	TArray<FString> EngineTestModules;

	/**
	 * Modules to load that have editor tests
	 */
	UPROPERTY(EditAnywhere, config, Category=Automation)
	TArray<FString> EditorTestModules;

	/**
	 * Folders containing levels to exclude from automated tests
	 */
	UPROPERTY(EditAnywhere, config, Category=Automation)
	TArray<FString> TestLevelFolders;

	/**
	 * Asset to test for open in automation process
	 */
	UPROPERTY(EditAnywhere, config, Category=ExternalTools)
	TArray<FExternalToolDefinition> ExternalTools;

	/**
	 * Asset import / Export test settings
	 */
	UPROPERTY(EditAnywhere, config, Category = Automation)
	TArray<FEditorImportExportTestDefinition> ImportExportTestDefinitions;
};
