// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SMessagingEndpointsFilterBar.h: Declares the SMessagingEndpointsFilterBar class.
=============================================================================*/

#pragma once


#define LOCTEXT_NAMESPACE "SMessagingEndpointsFilterBar"


/**
 * Implements the endpoints list filter bar widget.
 */
class SMessagingEndpointsFilterBar
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SMessagingEndpointsFilterBar) { }
	SLATE_END_ARGS()

public:

	/**
	 * Construct this widget
	 *
	 * @param InArgs - The declaration data for this widget.
	 * @param InFilter - The filter model.
	 */
	void Construct( const FArguments& InArgs, FMessagingDebuggerEndpointFilterRef InFilter )
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
						.HintText(LOCTEXT("SearchBoxHint", "Search endpoints"))
						.OnTextChanged(this, &SMessagingEndpointsFilterBar::HandleFilterStringTextChanged)
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

	// Holds a pointer to the filter model.
	FMessagingDebuggerEndpointFilterPtr Filter;
};


#undef LOCTEXT_NAMESPACE
