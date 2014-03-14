// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SMessagingMessageDetails.h: Declares the SMessagingMessageDetails class.
=============================================================================*/

#pragma once


/**
 * Implements the message details panel.
 */
class SMessagingMessageDetails
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SMessagingMessageDetails) { }
	SLATE_END_ARGS()

public:

	/**
	 * Destructor.
	 */
	~SMessagingMessageDetails( );

public:

	/**
	 * Construct this widget
	 *
	 * @param InArgs - The declaration data for this widget.
	 * @param InModel - The view model to use.
	 * @param InStyle - The visual style to use for this widget.
	 */
	void Construct( const FArguments& InArgs, const FMessagingDebuggerModelRef& InModel, const TSharedRef<ISlateStyle>& InStyle );

protected:

	/**
	 * Refreshes the details widget.
	 */
	void RefreshDetails( );

private:

	// Callback for generating a row widget for the dispatch state list view.
	TSharedRef<ITableRow> HandleDispatchStateListGenerateRow( FMessageTracerDispatchStatePtr DispatchState, const TSharedRef<STableViewBase>& OwnerTable );

	// Callback for getting the text of the 'Expiration' field.
	FString HandleExpirationText( ) const;

	// Callback for getting the text of the 'Message Type' field.
	FString HandleMessageTypeText( ) const;

	// Callback for handling message selection changes.
	void HandleModelSelectedMessageChanged( );

	// Callback for getting the text of the 'Sender' field.
	FString HandleSenderText( ) const;

	// Callback for getting the text of the 'Sender Thread' field.
	FString HandleSenderThreadText() const;

	// Callback for getting the text of the 'Timestamp' field.
	FString HandleTimestampText( ) const;

private:

	// Holds the list of dispatch states.
	TArray<FMessageTracerDispatchStatePtr> DispatchStateList;

	// Holds the dispatch state list view.
	TSharedPtr<SListView<FMessageTracerDispatchStatePtr> > DispatchStateListView;

	// Holds a pointer to the view model.
	FMessagingDebuggerModelPtr Model;

	// Holds the widget's visual style.
	TSharedPtr<ISlateStyle> Style;
};
