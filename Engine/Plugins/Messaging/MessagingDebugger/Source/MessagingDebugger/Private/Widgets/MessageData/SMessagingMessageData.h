// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SMessagingMessageData.h: Declares the SMessagingMessageData class.
=============================================================================*/

#pragma once


/**
 * Implements the message data panel.
 */
class SMessagingMessageData
	: public SCompoundWidget
	, public FNotifyHook
{
public:

	SLATE_BEGIN_ARGS(SMessagingMessageData) { }
	SLATE_END_ARGS()

public:

	/**
	 * Destructor.
	 */
	~SMessagingMessageData( );

public:

	/**
	 * Construct this widget
	 *
	 * @param InArgs - The declaration data for this widget.
	 * @param InModel - The view model to use.
	 * @param InStyle - The visual style to use for this widget.
	 */
	void Construct( const FArguments& InArgs, const FMessagingDebuggerModelRef& InModel, const TSharedRef<ISlateStyle>& InStyle );

public:

	// Begin FNotifyHook interface

	virtual void NotifyPostChange( const FPropertyChangedEvent& PropertyChangedEvent, class FEditPropertyChain* PropertyThatChanged ) OVERRIDE;

	// End FNotifyHook interface

private:

	// Handles checking whether the details view is enabled.
	bool HandleDetailsViewEnabled( ) const;

	// Handles determining the visibility of the details view.
	EVisibility HandleDetailsViewVisibility( ) const;

	// Callback for handling message selection changes.
	void HandleModelSelectedMessageChanged( );

private:

	// Holds the details view.
	TSharedPtr<IDetailsView> DetailsView;

	// Holds a pointer to the view model.
	FMessagingDebuggerModelPtr Model;

	// Holds the widget's visual style.
	TSharedPtr<ISlateStyle> Style;
};


#undef LOCTEXT_NAMESPACE
