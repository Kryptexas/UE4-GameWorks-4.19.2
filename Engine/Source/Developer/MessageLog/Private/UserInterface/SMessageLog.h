// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * An implementation for the message log widget.
 * It holds a series of message log listings which it can switch between.
 */
class SMessageLog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMessageLog){}
	SLATE_END_ARGS()

	static FName AppName;
	
	/** Constructor */
	SMessageLog();

	/** Destructor */	
	~SMessageLog();

	void Construct( const FArguments& InArgs, const TSharedPtr<class FMessageLogViewModel>& InMessageLogViewModel );

	/**
	 * Called after a key is pressed when this widget has keyboard focus
	 *
	 * @param  InKeyboardEvent  Keyboard event
	 *
	 * @return  Returns whether the event was handled, along with other possible actions
	 */
	FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent ) OVERRIDE;

private:
	/** Gets the currently visible log listing's label */
	FString GetCurrentListingLabel() const;

	/** Changes the selected log listing */
	void ChangeCurrentListingSelection( TSharedPtr<class FMessageLogListingViewModel> Selection, ESelectInfo::Type SelectInfo );

	/** Generates the widgets for the log listings in the current listing combo box */
	TSharedRef<SWidget> MakeCurrentListingSelectionWidget( TSharedPtr<class FMessageLogListingViewModel> Selection );

	/** Called whenever the viewmodel selection changes */
	void OnSelectionUpdated();

private:	

	/** The message log view model */
	TSharedPtr<class FMessageLogViewModel> MessageLogViewModel;

	/** The current log listing widget, if any */
	TSharedPtr<class SMessageLogListing> LogListing;

	/** The combo box for selecting the current listing */
	TSharedPtr< SComboBox< TSharedPtr<class FMessageLogListingViewModel> > > CurrentListingSelector;

	/** The widget for displaying the current listing */
	TSharedPtr<SBorder> CurrentListingDisplay;

	/** The widget for displaying the current listing's options, if any */
	TSharedPtr<SBorder> CurrentListingOptions;

	/**	Whether the view is currently updating the viewmodel selection */
	bool bUpdatingSelection;
};

