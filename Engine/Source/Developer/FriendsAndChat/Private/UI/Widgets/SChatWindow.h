// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SChatWindow.h: Declares the SChatWindow class.
=============================================================================*/

#pragma once

/**
 * Implements the FriendsList
 */
class SChatWindow
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SChatWindow) { }
		SLATE_ARGUMENT( const FFriendsAndChatStyle*, FriendStyle )
		SLATE_ARGUMENT( TSharedPtr< FFriendStuct >, FriendItem )
		SLATE_EVENT( FOnClicked, OnCloseClicked )
		SLATE_EVENT( FOnClicked, OnMinimizeClicked )
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the application.
	 * @param InArgs - The Slate argument list.
	 */
	void Construct( const FArguments& InArgs );

private:

	/**
	 * Handles the close button clicked.
	 */
	FReply CloseButton_OnClicked();

	/**
	 * Handles the minimize button clicked.
	 */
	FReply MinimizeButton_OnClicked();

	/**
	 * Handles the chat button clicked.
	 */
	FReply ChatButton_OnClicked();

	/**
	 * Handles generating the rows for chat.
	 * @param Message		- The chat message
	 * @param OwnerTable	- The owning table
	 * @return The generate table row
	 */
	TSharedRef< ITableRow > OnGenerateWidgetForChat( TSharedPtr< FFriendsAndChatMessage > Message, const TSharedRef< STableViewBase >& OwnerTable );

	/**
	 * Callback for when the chat list is updated - refreshes the list.
	 */
	void RefreshChatList();

private:

	// Holds the delegate for when the close button is clicked
	FOnClicked OnCloseClicked;
	// Holds the delegate for when the minimize button is clicked
	FOnClicked OnMinimizeClicked;
	// Holds the style to use when making the widget
	FFriendsAndChatStyle FriendStyle;
	// Holds the Chat list view
	TSharedPtr< SListView< TSharedPtr< FFriendsAndChatMessage > > > ChatListView;
	// Holds the Chat message list
	TArray< TSharedPtr< FFriendsAndChatMessage > > ChatItems;
	// Holds the chat box
	TSharedPtr< SEditableTextBox > ChatBox;
	// Holds a reference to the FriendItem data that is displayed in this row.
	TSharedPtr< FFriendStuct > FriendItem;
};

