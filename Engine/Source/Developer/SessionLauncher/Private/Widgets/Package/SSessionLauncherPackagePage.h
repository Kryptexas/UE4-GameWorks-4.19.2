// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherPackagePage.h: Declares the SSessionLauncherPackagePage class.
=============================================================================*/

#pragma once


/**
 * Implements the profile page for the session launcher wizard.
 */
class SSessionLauncherPackagePage
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncherPackagePage) { }
	SLATE_END_ARGS()


public:

	/**
	 * Destructor.
	 */
	~SSessionLauncherPackagePage( );


public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The Slate argument list.
	 * @param InModel - The data model.
	 */
	void Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel );


private:

	// Callback for getting the content text of the 'Cook Mode' combo button.
	FString HandlePackagingModeComboButtonContentText( ) const;

	// Callback for clicking an item in the 'Cook Mode' menu.
	void HandlePackagingModeMenuEntryClicked( ELauncherProfilePackagingModes::Type PackagingMode );

	// Callback for getting the visibility of the packaging settings area.
	EVisibility HandlePackagingSettingsAreaVisibility( ) const;

	// Callback for changing the selected profile in the profile manager.
	void HandleProfileManagerProfileSelected( const ILauncherProfilePtr& SelectedProfile, const ILauncherProfilePtr& PreviousProfile );


private:

	// Holds a pointer to the data model.
	FSessionLauncherModelPtr Model;
};
