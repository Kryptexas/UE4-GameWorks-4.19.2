// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherProjectPicker.h: Declares the SSessionLauncherProjectPicker class.
=============================================================================*/

#pragma once


/**
 * Implements the project loading area for the session launcher wizard.
 */
class SSessionLauncherProjectPicker
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncherProjectPicker) { }
	SLATE_END_ARGS()


public:

	/**
	 * Destructor.
	 */
	~SSessionLauncherProjectPicker( );


public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The Slate argument list.
	 * @param InModel - The data model.
	 */
	void Construct(	const FArguments& InArgs, const FSessionLauncherModelRef& InModel, bool InShowConfiguration = true );


protected:

	/**
	 * Creates the widget for the project menu.
	 *
	 * @return The widget.
	 */
	TSharedRef<SWidget> MakeProjectMenuWidget( );

	/**
	 * Creates the widget for the project selection.
	 *
	 * @return The widget.
	 */
	TSharedRef<SWidget> MakeProjectWidget( );

	/**
	 * Creates the widget for the project and configuration selection.
	 *
	 * @return The widget.
	 */
	TSharedRef<SWidget> MakeProjectAndConfigWidget( );

private:

	// Handles selecting a build configuration.
	void HandleBuildConfigurationSelectorConfigurationSelected( EBuildConfigurations::Type );

	// Handles getting the content text of the build configuration selector.
	FString HandleBuildConfigurationSelectorText( ) const;

	// Handles changing the selected profile in the profile manager.
	void HandleProfileManagerProfileSelected( const ILauncherProfilePtr& SelectedProfile, const ILauncherProfilePtr& PreviousProfile );

	// Handles getting the text for the project combo button.
	FString HandleProjectComboButtonText( ) const;

	// Handles getting the tooltip for the project combo button.
	FString HandleProjectComboButtonToolTip( ) const;

	// Handles clicking a project menu entry.
	void HandleProjectMenuEntryClicked( FString ProjectPath );

	// Handles determining the visibility of a validation error icon.
	EVisibility HandleValidationErrorIconVisibility( ELauncherProfileValidationErrors::Type Error ) const;

private:

	// Holds the list of available projects.
	TArray<TSharedPtr<FString> > ProjectList;

	// Holds a pointer to the data model.
	FSessionLauncherModelPtr Model;

	// Holds the repository path text box.
	TSharedPtr<SEditableTextBox> RepositoryPathTextBox;
};
