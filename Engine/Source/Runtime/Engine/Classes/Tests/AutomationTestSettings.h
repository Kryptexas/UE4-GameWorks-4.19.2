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
	UPROPERTY(config, EditAnywhere, Category=Automation)
	FFilePath AutomationTestmap;

	/**
	 * Asset to test for open in automation process
	 */
	UPROPERTY(EditAnywhere, config, Category=Automation)
	TArray<FOpenTestAsset> TestAssetsToOpen;
};
