// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSettingsEditor.h: Declares the SSettingsEditor class.
=============================================================================*/

#pragma once


/**
 * Implements an editor widget for settings.
 */
class SSettingsEditor
	: public SCompoundWidget
	, public FNotifyHook
{
public:

	SLATE_BEGIN_ARGS(SSettingsEditor) { }
	SLATE_END_ARGS()

public:

	/**
	 * Destructor.
	 */
	~SSettingsEditor( );

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 * @param InModel The view model.
	 */
	void Construct( const FArguments& InArgs, const ISettingsEditorModelRef& InModel );

public:

	// Begin SCompoundWidget interface

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;

	// End SCompoundWidget interface

public:

	// Begin FNotifyHook interface

	virtual void NotifyPostChange( const FPropertyChangedEvent& PropertyChangedEvent, class FEditPropertyChain* PropertyThatChanged ) OVERRIDE;

	// End FNotifyHook interface

protected:

	/**
	 * Checks out the default configuration file for the currently selected settings object.
	 *
	 * @return true if the check-out succeeded, false otherwise.
	 */
	bool CheckOutDefaultConfigFile( );

	/**
	 * Gets the absolute path to the Default.ini for the specified object.
	 *
	 * @param SettingsObject The object to get the path for.
	 * @return The path to the file.
	 */
	FString GetDefaultConfigFilePath( const TWeakObjectPtr<UObject>& SettingsObject ) const;

	/**
	 * Gets the settings object of the selected section, if any.
	 *
	 * @return The settings object.
	 */
	TWeakObjectPtr<UObject> GetSelectedSettingsObject( ) const;

	/**
	 * Creates a widget for the given settings category.
	 *
	 * @param Category The category to create the widget for.
	 * @return The created widget.
	 */
	TSharedRef<SWidget> MakeCategoryWidget( const ISettingsCategoryRef& Category );

	/**
	 * Makes the default configuration file for the currently selected settings object writable.
	 *
	 * @return true if it was made writable, false otherwise.
	 */
	bool MakeDefaultConfigFileWritable( );

	/**
	 * Reloads the settings categories.
	 */
	void ReloadCategories( );

	/**
	 * Shows a notification pop-up.
	 *
	 * @param Text The notification text.
	 * @param CompletionState The notification's completion state, i.e. success or failure.
	 */
	void ShowNotification( const FText& Text, SNotificationItem::ECompletionState CompletionState ) const;

private:

	// Callback for clicking the Back link.
	FReply HandleBackButtonClicked( );

	// Callback for clicking the 'Check Out' button.
	FReply HandleCheckOutButtonClicked( );

	// Callback for getting the widget index for the notice switcher.
	bool HandleConfigNoticeUnlocked( ) const;

	// Callback for when the user's culture has changed.
	void HandleCultureChanged( );

	// Callback for determining the visibility of the 'Locked' notice.
	EVisibility HandleDefaultConfigNoticeVisibility( ) const;

	// Callback for clicking the 'Reset to Defaults' button.
	FReply HandleExportButtonClicked( );

	// Callback for getting the enabled state of the 'Export' button.
	bool HandleExportButtonEnabled( ) const;

	// Callback for clicking the 'Reset to Defaults' button.
	FReply HandleImportButtonClicked( );

	// Callback for getting the enabled state of the 'Import' button.
	bool HandleImportButtonEnabled( ) const;

	// Callback for changing the selected settings section.
	void HandleModelSelectionChanged( );

	// Callback for clicking the 'Reset to Defaults' button.
	FReply HandleResetDefaultsButtonClicked( );

	// Callback for getting the enabled state of the 'Reset to Defaults' button.
	bool HandleResetToDefaultsButtonEnabled( ) const;

	// Callback for navigating a settings section link.
	void HandleSectionLinkNavigate( ISettingsSectionPtr Section );

	// Callback for getting the visibility of a section link image.
	EVisibility HandleSectionLinkImageVisibility( ISettingsSectionPtr Section ) const;

	// Callback for clicking the 'Set as Defaults' button.
	FReply HandleSetAsDefaultButtonClicked( );

	// Callback for getting the enabled state of the 'Set as Defaults' button.
	bool HandleSetAsDefaultButtonEnabled( ) const;

	// Callback for getting the section description text.
	FText HandleSettingsBoxDescriptionText( ) const;

	// Callback for getting the section text for the settings box.
	FString HandleSettingsBoxTitleText( ) const;

	// Callback for determining the visibility of the settings box.
	EVisibility HandleSettingsBoxVisibility( ) const;

	// Callback for the modification of categories in the settings container.
	void HandleSettingsContainerCategoryModified( const FName& CategoryName );

	// Callback for checking whether the settings view is enabled.
	bool HandleSettingsViewEnabled( ) const;

	// Callback for determining the visibility of the settings view.
	EVisibility HandleSettingsViewVisibility( ) const;

private:

	// Holds the vertical box for settings categories.
	TSharedPtr<SVerticalBox> CategoriesBox;

	// Holds a flag indicating whether the section's configuration file needs to be checked out.
	bool DefaultConfigCheckOutNeeded;

	// Holds a timer for checking whether the section's configuration file needs to be checked out.
	float DefaultConfigCheckOutTimer;

	// Holds the overlay slot for custom widgets.
	SOverlay::FOverlaySlot* CustomWidgetSlot;

	// Holds the path to the directory that the last settings were exported to.
	FString LastExportDir;

	// Holds a pointer to the view model.
	ISettingsEditorModelPtr Model;

	// Holds the settings container.
	ISettingsContainerPtr SettingsContainer;

	// Holds the details view.
	TSharedPtr<IDetailsView> SettingsView;
};
