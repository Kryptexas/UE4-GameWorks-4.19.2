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

	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs );

private:

	// Callback for determining the visibility of the check-out button.
	EVisibility HandleCheckOutButtonVisibility( ) const;

	// Callback for getting the widget index for the notice switcher.
	int32 HandleNoticeSwitcherWidgetIndex( ) const
	{
		return bIsUnlocked.Get() ? 1 : 0;
	}

	// Callback for getting the visibility of the 'Source control unavailable' text block.
	FText HandleSccUnavailableTextBlockText( ) const;

private:

	TAttribute<bool> bIsUnlocked;
};
