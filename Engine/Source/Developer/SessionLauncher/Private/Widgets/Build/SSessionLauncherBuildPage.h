// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSessionLauncherBuildPage.h: Declares the SSessionLauncherBuildPage class.
=============================================================================*/

#pragma once


/**
 * Implements the profile page for the session launcher wizard.
 */
class SSessionLauncherBuildPage
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSessionLauncherBuildPage) { }
	SLATE_END_ARGS()


public:

	/**
	 * Destructor.
	 */
	~SSessionLauncherBuildPage( );


public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs - The Slate argument list.
	 * @param InModel - The data model.
	 */
	void Construct(	const FArguments& InArgs, const FSessionLauncherModelRef& InModel );


private:

	// Callback for changing the checked state of a platform menu check box. 
	void HandleBuildCheckedStateChanged( ESlateCheckBoxState::Type CheckState );

	// Callback for determining whether a platform menu entry is checked.
	ESlateCheckBoxState::Type HandleBuildIsChecked() const;

	// Callback for changing the selected profile in the profile manager.
	void HandleProfileManagerProfileSelected( const ILauncherProfilePtr& SelectedProfile, const ILauncherProfilePtr& PreviousProfile );

	// Callback for determining if the build platform list should be displayed
	EVisibility HandleBuildPlatformVisibility( ) const;

    // Callback for pressing the Advanced Setting - Generate DSYM button.
    FReply HandleGenDSYMClicked();
    bool HandleGenDSYMButtonEnabled() const;
    bool GenerateDSYMForProject( const FString& ProjectName, const FString& Configuration );

private:

	// Holds a pointer to the data model.
	FSessionLauncherModelPtr Model;
};
