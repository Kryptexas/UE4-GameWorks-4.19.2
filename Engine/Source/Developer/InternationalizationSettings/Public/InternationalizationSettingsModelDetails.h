// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "InternationalizationSettingsModel.h"

class INTERNATIONALIZATIONSETTINGS_API FInternationalizationSettingsModelDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** Default Constructor */
	FInternationalizationSettingsModelDetails()
		:	RequiresRestart(false)
	{
	}

	/** Destructor */
	~FInternationalizationSettingsModelDetails();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailLayout ) OVERRIDE;

private:

	void OnSettingsChanged();

	/** Prompt the user to restart the editor */
	void PromptForRestart() const;

	/** Look for available cultures */
	void RefreshAvailableCultures();

	/** Look for available languages */
	void RefreshAvailableLanguages();

	/** Look for available regions */
	void RefreshAvailableRegions();

	/** Delegate called to display the current language */
	FText GetCurrentLanguageText() const;

	/** Generate a widget for the language combo */
	TSharedRef<SWidget> OnLanguageGenerateWidget( TSharedPtr<FCulture> Culture, IDetailLayoutBuilder* DetailBuilder ) const;

	/** Delegate called when the language selection has changed */
	void OnLanguageSelectionChanged( TSharedPtr<FCulture> Culture, ESelectInfo::Type SelectionType );

	/** Generate a widget for the region combo */
	TSharedRef<SWidget> OnRegionGenerateWidget( TSharedPtr<FCulture> Culture, IDetailLayoutBuilder* DetailBuilder ) const;

	/** Delegate called to display the current region */
	FText GetCurrentRegionText() const;

	/** Delegate called when the region selection has changed */
	void OnRegionSelectionChanged( TSharedPtr<FCulture> Culture, ESelectInfo::Type SelectionType );

	/** Delegate called to see if we're allowed to change the region */
	bool IsRegionSelectionAllowed() const;

	/** Delegate called to determine whether to collapse or display the restart row. */
	EVisibility GetInternationalizationRestartRowVisibility() const;

	/** Write to config now the Editor is shutting down (all packages are saved) */
	void HandleShutdownPostPackagesSaved();

private:
	TWeakObjectPtr<UInternationalizationSettingsModel> Model;

	/** The culture selected at present */
	TSharedPtr<FCulture> SelectedCulture;

	/** The language selected at present */
	TSharedPtr<FCulture> SelectedLanguage;

	/** The cultures we have available to us */
	TArray< TSharedPtr<FCulture> > AvailableCultures;

	/** The languages we have available to us */
	TArray< TSharedPtr<FCulture> > AvailableLanguages;

	/** The dropdown for the languages */
	TSharedPtr<SComboBox< TSharedPtr<FCulture> >> LanguageComboBox;

	/** The regions we have available to us */
	TArray< TSharedPtr<FCulture> > AvailableRegions;

	/** The dropdown for the regions */
	TSharedPtr<SComboBox< TSharedPtr<FCulture> >> RegionComboBox;

	bool RequiresRestart;
};