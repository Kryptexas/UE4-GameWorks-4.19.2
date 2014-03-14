// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SFriendsListContextMenu.h: Declares the SFriendsListContextMenu class.
=============================================================================*/

#pragma once

class SFriendsListContextMenu
	: public SCompoundWidget
{
	SLATE_BEGIN_ARGS( SFriendsListContextMenu )
		: _FriendItem()
	{ }
	SLATE_ARGUMENT( TSharedPtr< FFriendStuct >, FriendItem )
	SLATE_ARGUMENT( const FFriendsAndChatStyle*, FriendStyle )
	SLATE_END_ARGS()

	/**
	 * Constructs the application.
	 *
	 * @param InArgs - The Slate argument list.
	 */
	void Construct( const FArguments& InArgs );

private:

	/**
	 * Make the context menu.
	 * @return The context menu widget
	 */
	TSharedRef<SWidget> MakeContextMenu();

	/**
	 * Handle the Add Friend button clicked.
	 */
	void OnAddFriendClicked();

	/**
	 * Handle the chat clicked.
	 */
	void OnChatClicked();

	/**
	 * Handle the remove friend clicked.
	 */
	void OnRemoveFriendClicked();

	/**
	 * Handle the remove friend clicked.
	 */
	void OnInviteToGameClicked();

	/**
	 * Handle join game clicked.
	 */
	void OnJoinGameClicked();

	/**
	 * Handle accept friend clicked.
	 */
	void OnAcceptFriendClicked();

	/**
	 * Handle reject friend clicked.
	 */
	void OnRejectFriendClicked();

public:
	// Holds a reference to the news message that is displayed in this row.
	TSharedPtr< FFriendStuct > FriendItem;
};

