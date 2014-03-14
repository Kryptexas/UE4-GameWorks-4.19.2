// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SFriendsList.h: Declares the SFriendsList class.
=============================================================================*/

#pragma once

/**
 * Implements the FriendsList
 */
class SFriendsList
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SFriendsList) { }
		SLATE_ARGUMENT( const FFriendsAndChatStyle*, FriendStyle )
		SLATE_EVENT( FOnClicked, OnCloseClicked )
		SLATE_EVENT( FOnClicked, OnMinimizeClicked )
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the application.
	 *
	 * @param InArgs - The Slate argument list.
	 */
	void Construct( const FArguments& InArgs );

private:

	/**
	 * Generates a widget for a Friend item.
	 * @param InItem - the FriendItem
	 * @param OwnerTable - the owning table
	 * @return The table row widget
	 */
	TSharedRef<ITableRow> HandleGenerateFriendWidget( TSharedPtr< FFriendStuct > InItem, const TSharedRef<STableViewBase>& OwnerTable );

	/**
	 * Callback for when the friends list is updated - refreshes the list.
	 */
	void RefreshFriendsList();

	/**
	 * Get the list count as text.
	 * @return The number of friends
	 */
	FString GetListCountText() const;

	/**
	 * Handles the search for friend button clicked.
	 */
	FReply SearchFriend_OnClicked();

	/** 
	 * Handle friend search committed 
	 * @param CommentText - the entered text
	 * @param CommitInfo - the commit type
	 */
	void HandleFriendEntered(const FText& CommentText, ETextCommit::Type CommitInfo);

	/**
	 * Handles the close button clicked.
	 */
	FReply CloseButton_OnClicked();

	/**
	 * Handles the minimize button clicked.
	 */
	FReply MinimizeButton_OnClicked();

	/**
	 * Handles the default list selection button change.
	 * @param Check State - The checkstate
	 */
	void DefaultListSelect_OnChanged( ESlateCheckBoxState::Type CheckState );

	/**
	 * Get the check state for the default list selection check box.
	 * @return The checkstate
	 */
	ESlateCheckBoxState::Type DefaultListSelect_IsChecked() const;

	/**
	 * Handles the recent players list selection button change.
	 * @param Check State - The checkstate
	 */
	void RecentListSelect_OnChanged( ESlateCheckBoxState::Type CheckState );

	/**
	 * Handles the player requests list selection button change.
	 * @param Check State - The checkstate.
	 */
	void RequestListSelect_OnChanged( ESlateCheckBoxState::Type CheckState );

	/**
	 * Get the check state for the recent players list selection check box.
	 * @return The checkstate
	 */
	ESlateCheckBoxState::Type RecentListSelect_IsChecked();

	/**
	 * Set the toggle button state depending on the selected list.
	 */
	void UpdateButtonState();

	/**
	 * Get the menu text color depending on the selected list.
	 * @param ListType - the list to get the color for
	 * @return The list color
	 */
	FSlateColor GetMenuTextColor( EFriendsDisplayLists::Type ListType ) const;

private:

	// Holds the list of friends
	TArray< TSharedPtr< FFriendStuct > > FriendsList;
	// Holds the list of outgoing invites
	TArray< TSharedPtr< FFriendStuct > > OutgoingFriendsList;
	// Holds the tree view of the Friends list
	TSharedPtr< SListView< TSharedPtr< FFriendStuct > > > FriendsListView;
	// Holds the style to use when making the widget
	FFriendsAndChatStyle FriendStyle;
	// Holds the delegate for when the close button is clicked
	FOnClicked OnCloseClicked;
	// Holds the delegate for when the minimize button is clicked
	FOnClicked OnMinimizeClicked;
	// Holds the text box used to enter the name of friend to search for
	TSharedPtr< SEditableTextBox > FriendNameTextBox;
	// Holds the recent players check box
	TSharedPtr< SCheckBox > RecentPlayersButton;
	// Holds the recent players check box
	TSharedPtr< SCheckBox > FriendRequestsButton;
	// Holds the default friends check box
	TSharedPtr< SCheckBox > DefaultPlayersButton;
	// Holds the default list name
	EFriendsDisplayLists::Type CurrentList;
	// Holds the friends list display box.
	TSharedPtr< SVerticalBox > DisplayBox;
};

