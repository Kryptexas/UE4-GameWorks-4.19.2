// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SMessagingTypesFilterBar.h: Declares the SMessagingTypesFilterBar class.
=============================================================================*/

#pragma once


#define LOCTEXT_NAMESPACE "SMessagingTypesFilterBar"


/**
 * Implements the message type list filter bar widget.
 */
class SMessagingTypesFilterBar
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SMessagingTypesFilterBar) { }

		/**
		 * Called when the filter settings have changed.
		 */
		SLATE_EVENT(FSimpleDelegate, OnFilterChanged)

	SLATE_END_ARGS()

public:

	/**
	 * Default constructor.
	 */
	SMessagingTypesFilterBar( )
		: Filter(NULL)
	{ }

public:

	/**
	 * Construct this widget
	 *
	 * @param InArgs - The declaration data for this widget.
	 * @param InFilter - The filter model.
	 */
	void Construct( const FArguments& InArgs, FMessagingDebuggerTypeFilterRef InFilter )
	{
		Filter = InFilter;

		ChildSlot
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Top)
				[
					// search box
					SNew(SSearchBox)
						.HintText(LOCTEXT("SearchBoxHint", "Search message types"))
						.OnTextChanged(this, &SMessagingTypesFilterBar::HandleFilterStringTextChanged)
				]
		];
	}

private:

	// Handles changing the filter string text box text.
	void HandleFilterStringTextChanged( const FText& NewText )
	{
		Filter->SetFilterString(NewText.ToString());
	}

private:

	// Holds the filter model.
	FMessagingDebuggerTypeFilterPtr Filter;
};


#undef LOCTEXT_NAMESPACE
