// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "InternationalizationSettingsModel.h"

class INTERNATIONALIZATIONSETTINGS_API FInternationalizationSettingsModelDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** Default Constructor */
	FInternationalizationSettingsModelDetails()
		: RequiresRestart(false)
		, IsMakingChangesToModel(false)
	{
	}

	/** Destructor */
	~FInternationalizationSettingsModelDetails();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailLayout ) override;

private:
	void UpdateInternalStateFromSettingsModel();

	void OnSettingsChanged();

	/** Look for available editor culture regions */
	void RefreshAvailableEditorRegions();

	/** Delegate called to display the current editor language */
	FText GetEditorCurrentLanguageText() const;

	/** Delegate called when the editor language selection has changed */
	void OnEditorLanguageSelectionChanged( FCulturePtr Culture, ESelectInfo::Type SelectionType );

	/** Delegate called to display the current editor region */
	FText GetCurrentEditorRegionText() const;

	/** Delegate called when the editor region selection has changed */
	void OnEditorRegionSelectionChanged( FCulturePtr Culture, ESelectInfo::Type SelectionType );

	/** Delegate called to see if we're allowed to change the region */
	bool IsEditorRegionSelectionAllowed() const;

	/** Look for available game culture regions */
	void RefreshAvailableNativeGameRegions();

	/** Delegate called to display the current native game language */
	FText GetNativeGameCurrentLanguageText() const;

	/** Delegate called when the native game language selection has changed */
	void OnNativeGameLanguageSelectionChanged( FCulturePtr Culture, ESelectInfo::Type SelectionType );

	/** Delegate called to display the current native game region */
	FText GetCurrentNativeGameRegionText() const;

	/** Delegate called when the native game region selection has changed */
	void OnNativeGameRegionSelectionChanged( FCulturePtr Culture, ESelectInfo::Type SelectionType );

	/** Delegate called to see if we're allowed to change the region */
	bool IsNativeGameRegionSelectionAllowed() const;

	/** Generate a widget for the language combo */
	TSharedRef<SWidget> OnLanguageGenerateWidget( FCulturePtr Culture, IDetailLayoutBuilder* DetailBuilder ) const;

	/** Generate a widget for the region combo */
	TSharedRef<SWidget> OnRegionGenerateWidget( FCulturePtr Culture, IDetailLayoutBuilder* DetailBuilder ) const;

	/** Delegate called to determine whether to collapse or display the restart row. */
	EVisibility GetInternationalizationRestartRowVisibility() const;

	/** Delegate called when the the checked state of whether or not field names should be localized has changed. */
	void ShouldLoadLocalizedFieldNamesCheckChanged(ECheckBoxState CheckState);

	/** Delegate called when the the checked state of whether or not nodes and pins in graph editors should be localized has changed. */
	void ShouldShowNodesAndPinsUnlocalized(ECheckBoxState CheckState);

private:
	TWeakObjectPtr<UInternationalizationSettingsModel> Model;

	/** The culture selected at present */
	FCulturePtr SelectedEditorCulture;

	/** The language selected at present */
	FCulturePtr SelectedEditorLanguage;

	/** The cultures we have available to us */
	TArray< FCulturePtr > AvailableEditorCultures;

	/** The languages we have available to us */
	TArray< FCulturePtr > AvailableEditorLanguages;

	/** The dropdown for the languages */
	TSharedPtr<SComboBox< FCulturePtr > > EditorLanguageComboBox;

	/** The regions we have available to us */
	TArray< FCulturePtr > AvailableEditorRegions;

	/** The dropdown for the regions */
	TSharedPtr<SComboBox< FCulturePtr > > EditorRegionComboBox;

	/** The culture selected at present */
	FCulturePtr SelectedNativeGameCulture;

	/** The language selected at present */
	FCulturePtr SelectedNativeGameLanguage;

	/** The cultures we have available to us */
	TArray< FCulturePtr > AvailableNativeGameCultures;

	/** The languages we have available to us */
	TArray< FCulturePtr > AvailableNativeGameLanguages;

	/** The dropdown for the languages */
	TSharedPtr<SComboBox< FCulturePtr > > NativeGameLanguageComboBox;

	/** The regions we have available to us */
	TArray< FCulturePtr > AvailableNativeGameRegions;

	/** The dropdown for the regions */
	TSharedPtr<SComboBox< FCulturePtr > > NativeGameRegionComboBox;

	/** The check box for using localized property names */
	TSharedPtr<SCheckBox> LocalizedPropertyNamesCheckBox;

	/** The check box for showing nodes and pins unlocalized */
	TSharedPtr<SCheckBox> UnlocalizedNodesAndPinsCheckBox;

	bool RequiresRestart;

	bool IsMakingChangesToModel;
};