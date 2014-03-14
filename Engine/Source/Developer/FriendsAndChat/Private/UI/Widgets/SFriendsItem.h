// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SFriendsItem.h: Declares the SFriendsItem class.
	Displays a line in the friends list view 
=============================================================================*/

#pragma once

// Enum to list friend actions
namespace EFriendsActionType
{
	enum Type
	{
		Add_Friend, 	// Send a friend request
		Delete_Friend,	// Delete an existing friend
		Cancel_Request,	// Cancel an outgoing request
		Invite_To_Game,	// Invite a friend to your game
		Join_Game,		// Attempt to join a friends game
		Reject_Friend,	// Reject an incoming friend request
		Accept_Friend,	// Accept an incoming friend request
	};
};

class SFriendsItem
	: public STableRow< TSharedPtr<FFriendStuct> >
{
public:
	SLATE_BEGIN_ARGS( SFriendsItem )
		: _FriendItem()
	{ }
	SLATE_ARGUMENT( TSharedPtr< FFriendStuct >, FriendItem )
	SLATE_ARGUMENT( const FFriendsAndChatStyle*, FriendStyle )
	SLATE_END_ARGS()

	/**
	 * Constructs the widget.
	 * @param InArgs - The Slate argument list.
	 * @param InOwnerTableView - The owning table.
	 */
	void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView );

private:

	/**
	 * Handle a menu option selected.
	 * @param FriendActionType - the action type.
	 */	
	FReply OnOptionSelected( EFriendsActionType::Type FriendActionType );

	/**
	* Returns the text representation of an action.
	* @param FriendActionType - The action type.
	* @return A text value.
	*/
	const FText& GetActionText( EFriendsActionType::Type FriendActionType );

	/**
	* Generate the menu widget.
	* @return A menu widget.
	*/
	TSharedRef<SWidget> OnGetMenuContent();

private:
	// Holds a reference to the FriendItem data that is displayed in this row.
	TSharedPtr< FFriendStuct > FriendItem;

	// Holds the style to use when making the widget.
	FFriendsAndChatStyle FriendStyle;

	// Holds the menu button.
	TSharedPtr< SComboButton > MenuButton; 
};

