// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "InternationalizationSettingsModel.generated.h"


/**
 * Implements loading and saving of internationalization settings.
 */
UCLASS()
class INTERNATIONALIZATIONSETTINGS_API UInternationalizationSettingsModel
	:	public UObject
{
	GENERATED_UCLASS_BODY()

public:

	void SaveDefaults();
	void ResetToDefault();
	FString GetEditorCultureName() const;
	void SetEditorCultureName(const FString& CultureName);
	FString GetNativeGameCultureName() const;
	void SetNativeGameCultureName(const FString& CultureName);
	bool ShouldLoadLocalizedPropertyNames() const;
	void ShouldLoadLocalizedPropertyNames(const bool Value);
	bool ShouldShowNodesAndPinsUnlocalized() const;
	void ShouldShowNodesAndPinsUnlocalized(const bool Value);
public:

	/**
	 * Returns an event delegate that is executed when a setting has changed.
	 *
	 * @return The delegate.
	 */
	DECLARE_EVENT(UInternationalizationSettingsModel, FSettingsChangedEvent);
	FSettingsChangedEvent& OnSettingsChanged( ) { return SettingsChangedEvent; }

private:

	// Holds an event delegate that is executed when a setting has changed.
	FSettingsChangedEvent SettingsChangedEvent;
};
