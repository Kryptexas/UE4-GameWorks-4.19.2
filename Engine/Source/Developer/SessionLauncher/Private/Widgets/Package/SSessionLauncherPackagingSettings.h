// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherPackagingSettings.h: Declares the SSessionLauncherPackagingSettings class.
=============================================================================*/

#pragma once


/**
 * Implements the packaging settings panel.
 */
class SSessionLauncherPackagingSettings
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncherPackagingSettings) { }
	SLATE_END_ARGS()


public:

	/**
	 * Destructor.
	 */
	~SSessionLauncherPackagingSettings( );


public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The Slate argument list.
	 * @param InModel - The data model.
	 */
	void Construct( const FArguments& InArgs, const FSessionLauncherModelRef& InModel );


private:

	// Callback for changing the selected profile in the profile manager.
	void HandleProfileManagerProfileSelected( const ILauncherProfilePtr& SelectedProfile, const ILauncherProfilePtr& PreviousProfile );


private:

	// Holds a pointer to the data model.
	FSessionLauncherModelPtr Model;

	// Holds the repository path text box.
	TSharedPtr<SEditableTextBox> RepositoryPathTextBox;
};
