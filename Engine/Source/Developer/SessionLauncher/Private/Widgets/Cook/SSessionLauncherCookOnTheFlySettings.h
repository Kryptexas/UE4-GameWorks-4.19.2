// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherCookOnTheFlySettings.h: Declares the SSessionLauncherCookOnTheFlySettings class.
=============================================================================*/

#pragma once


/**
 * Implements the cook-by-the-book settings panel.
 */
class SSessionLauncherCookOnTheFlySettings
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncherCookOnTheFlySettings) { }
	SLATE_END_ARGS()


public:

	/**
	 * Destructor.
	 */
	~SSessionLauncherCookOnTheFlySettings( );


public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The Slate argument list.
	 * @param InModel - The data model.
	 */
	void Construct(	const FArguments& InArgs, const FSessionLauncherModelRef& InModel );


private:

	// Callback for check state changes of the 'Incremental' check box.
	void HandleIncrementalCheckBoxCheckStateChanged( ESlateCheckBoxState::Type NewState );

	// Callback for determining the checked state of the 'Incremental' check box.
	ESlateCheckBoxState::Type HandleIncrementalCheckBoxIsChecked( ) const;

	// Callback for changing the selected profile in the profile manager.
	void HandleProfileManagerProfileSelected( const ILauncherProfilePtr& SelectedProfile, const ILauncherProfilePtr& PreviousProfile );

	// Callback for determining the visibility of a validation error icon.
	EVisibility HandleValidationErrorIconVisibility( ELauncherProfileValidationErrors::Type Error ) const;


private:

	// Holds a pointer to the data model.
	FSessionLauncherModelPtr Model;
};
