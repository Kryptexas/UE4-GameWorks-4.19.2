// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	FriendListItems.h: Declares structs and enmums used in the FriendsAndChat module.
=============================================================================*/

#pragma once

// Class containing the friend information - used to build the list view
class FFriendStuct
{
public:

	/**
	 * Constructor takes the required details
	 *
	 * @param InOnlineFriend		The online friend
	 * @param InOnlineUser			The online user
	 * @param InListType			The list type
	 */
	FFriendStuct( TSharedPtr< FOnlineFriend > InOnlineFriend, TSharedPtr< FOnlineUser > InOnlineUser, EFriendsDisplayLists::Type InListType )
		: GroupName(TEXT(""))
		, OnlineFriend( InOnlineFriend )
		, OnlineUser( InOnlineUser )
		, UniqueID( InOnlineUser->GetUserId() )
		, ListType( InListType )
	{}

	/**
	 * Constructor takes the required details
	 *
	 * @param InGroupName		The group name
	 */
	FFriendStuct( const FString& InGroupName )
		: GroupName( InGroupName )
	{}

	// Equality operator checks friend stuct
	friend FORCEINLINE bool operator==(const FFriendStuct& X, const FFriendStuct& Y)
	{
		if ( X.OnlineFriend.IsValid() && Y.OnlineFriend.IsValid() )
		{
			return ( X.OnlineFriend->GetInviteStatus() == Y.OnlineFriend->GetInviteStatus() ) 
				&& ( X.GetUniqueID().Get() == Y.GetUniqueID().Get() );
		}
		
		return X.GroupName == Y.GroupName;
	}

	friend FORCEINLINE bool operator!=(const FFriendStuct& A, const FFriendStuct& B)
	{
		return !(A == B);
	}

public:

	/**
	 * Add a child node to this friend.
	 * @param InChild - the child node.
	 */
	void AddChild( TSharedPtr< FFriendStuct > InChild );

	/**
	 * Get the on-line user associated with this account.
	 * @return The online user.
	 */
	const TSharedPtr< FOnlineUser > GetOnlineUser() const;

	/**
	 * Get the cached on-line Friend.
	 * @return The online friend.
	 */
	const TSharedPtr< FOnlineFriend > GetOnlineFriend() const;

	/**
	 * Get the friend list.
	 * @return The array of child nodes.
	 */
	TArray< TSharedPtr < FFriendStuct > >& GetChildList();

	/**
	 * Get the cached user name.
	 * @return The user name.
	 */
	const FString GetName() const;

	/**
	 * Get the Unique ID.
	 * @return The Unique Net ID.
	 */
	const TSharedRef< FUniqueNetId > GetUniqueID() const;

	/**
	 * Is this friend in the default list.
	 * @return The List Type.
	 */
	const EFriendsDisplayLists::Type GetListType() const;

	/**
	 * Set new online friend.
	 * @param InOnlineFriend - the new online friend.
	 */
	void SetOnlineFriend( TSharedPtr< FOnlineFriend > InOnlineFriend );

	/**
	 * Clear updated flag.
	 */
	void ClearUpdated();

	/**
	 * Check if we have been updated.
	 * @return true if updated
	 */
	bool IsUpdated();

	/**
	 * Set if pending invitation response.
	 */
	void SetPendingInvite();

	/**
	 * Set if pending invitation response.
	 */
	void SetPendingAccept();

	/**
	 * Get the invitation status.
	 * @return The invitation status
	 */
	EInviteStatus::Type GetInviteStatus();

private:
	// hide the default constructor
	FFriendStuct(): bIsUpdated(true), GroupName(TEXT("")) {};

private:
	// Holds if this item has been updated
	bool bIsUpdated;
	// Hold the child list array
	TArray< TSharedPtr< FFriendStuct > > Children;
	// Holds the group name
	const FString GroupName;
	// Holds the cached online friend
	TSharedPtr<FOnlineFriend> OnlineFriend;
	// Holds the cached online user
	const TSharedPtr<FOnlineUser> OnlineUser;
	// Holds the cached user id
	TSharedPtr< FUniqueNetId > UniqueID;
	// Holds if this is the list type
	EFriendsDisplayLists::Type ListType;
	// Holds if we are pending an accept as friend action
	bool bIsPendingAccepted;
	// Holds if we are pending an invite response
	bool bIsPendingInvite;
};
