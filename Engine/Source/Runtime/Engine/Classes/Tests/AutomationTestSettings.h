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
	 * Asset to test for open in automation process
	 */
	UPROPERTY(EditAnywhere, config, Category=ExternalTools)
	TArray<FExternalToolDefinition> ExternalTools;
};
