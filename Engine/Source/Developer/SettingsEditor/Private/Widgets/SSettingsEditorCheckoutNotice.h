// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SSettingsEditorCheckoutNotice.h: Declares the SSettingsEditorCheckoutNotice class.
=============================================================================*/

/**
 * Implements a widget that provides a notice for files that need to be checked out.
 */
class SSettingsEditorCheckoutNotice
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSettingsEditorCheckoutNotice) { }

		/** Called to determine if the associated file is unlocked */
		SLATE_ATTRIBUTE(bool, Unlocked)
	
		/** Slot for this button's content (optional) */
		SLATE_NAMED_SLOT(FArguments, LockedContent)

		/** Called when the 'Check Out' button is clicked */
		SLATE_EVENT(FOnClicked, OnCheckOutClicked)

	SLATE_END_ARGS()

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The construction arguments.
	 */
	void Construct( const FArguments& InArgs );

private:

	// Callback for clicking the 'Check Out' button.
	FReply HandleCheckOutButtonClicked( );

	// Callback for getting the text of the 'Check Out' button.
	FText HandleCheckOutButtonText( ) const;

	// Callback for getting the tool tip text of the 'Check Out' button.
	FText HandleCheckOutButtonToolTip( ) const;

	// Callback for determining the visibility of the check-out button.
	EVisibility HandleCheckOutButtonVisibility( ) const;

	// Callback for getting the widget index for the notice switcher.
	int32 HandleNoticeSwitcherWidgetIndex( ) const
	{
		return bIsUnlocked.Get() ? 1 : 0;
	}

private:

	// Holds a delegate that is executed when the 'Check Out' button has been clicked.
	FOnClicked CheckOutClickedDelegate;

	TAttribute<bool> bIsUnlocked;
};
