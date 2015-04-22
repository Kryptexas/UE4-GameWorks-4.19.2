// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DeveloperSettings.generated.h"

/** The different types of settings container that can be used. */
UENUM()
enum class EDeveloperSettingsContainer : uint8
{
	Project,
	Editor
};

/**
 * The base class of any auto discovered settings object.
 */
UCLASS(abstract)
class ENGINE_API UDeveloperSettings : public UObject
{
	GENERATED_BODY()

public:
	UDeveloperSettings(const FObjectInitializer& ObjectInitializer);

	/** Gets the settings container name for the settings, either Project or Editor */
	FName GetContainerName() const;
	/** Gets the category for the settings, some high level grouping like, Editor, Engine, Game...etc. */
	FName GetCategoryName() const;
	/** The unique name for your section of settings, uses the class's FName. */
	FName GetSectionName() const;

#if WITH_EDITOR
	/** Gets the section text, uses the classes DisplayName by default. */
	virtual FText GetSectionText() const;
	/** Gets the description for the section, uses the classes ToolTip by default. */
	virtual FText GetSectionDescription() const;
#endif

	/** Gets a custom widget for the settings.  This is only for very custom situations. */
	virtual TSharedPtr<SWidget> GetCustomSettingsWidget() const;

private:
	/**
	 * The category name to use, overrides the one detected by looking at the config=... class metadata.
	 * Arbitrary category names are not supported, this must map to an existing category we support in the settings
	 * viewer.
	 */
	FName CategoryName;

	/** The container for these settings. */
	EDeveloperSettingsContainer Container;
};
