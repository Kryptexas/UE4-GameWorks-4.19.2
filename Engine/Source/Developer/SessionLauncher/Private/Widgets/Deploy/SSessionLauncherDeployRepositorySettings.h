// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherDeployRepositorySettings.h: Declares the SSessionLauncherDeployRepositorySettings class.
=============================================================================*/

#pragma once


/**
 * Implements the deploy-to-device settings panel.
 */
class SSessionLauncherDeployRepositorySettings
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncherDeployRepositorySettings) { }
	SLATE_END_ARGS()


public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The Slate argument list.
	 * @param InModel - The data model.
	 */
	void Construct(	const FArguments& InArgs, const FSessionLauncherModelRef& InModel );


private:

	FReply HandleBrowseButtonClicked( );

	/** Handles entering in a command */
	void OnTextCommitted(const FText& InText, ETextCommit::Type CommitInfo);

	void OnTextChanged(const FText& InText);

private:

	// Holds a pointer to the data model.
	FSessionLauncherModelPtr Model;

	// Holds the repository path text box.
	TSharedPtr<SEditableTextBox> RepositoryPathTextBox;
};
