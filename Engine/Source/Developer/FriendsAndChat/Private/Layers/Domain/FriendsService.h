// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
* Implement the Friends data access
*/
class FFriendsService
	: public TSharedFromThis<FFriendsService>
{
public:

	/** Destructor. */
	virtual ~FFriendsService() {};

	virtual void Login() = 0;
	virtual void Logout() = 0;


	/**
	* Get the friends filtered list of friends.
	*
	* @param OutFriendsList  Array of friends to fill in.
	* @return the friend list count.
	*/
	virtual int32 GetFilteredFriendsList(TArray< TSharedPtr< IFriendItem > >& OutFriendsList) = 0;

	/**
	* Get the recent players list.
	* @return the list.
	*/
	virtual TArray< TSharedPtr< IFriendItem > >& GetRecentPlayerList() = 0;

	/**
	* Accept a friend request.
	*
	* @param FriendItem The friend to accept.
	*/
	virtual void AcceptFriend(TSharedPtr< IFriendItem > FriendItem) = 0;

	/**
	* Reject a friend request.
	*
	* @param FriendItem The friend to reject.
	*/
	virtual void RejectFriend(TSharedPtr< IFriendItem > FriendItem) = 0;

	/**
	* Request a friend be added.
	*
	* @param FriendName The friend name.
	*/
	virtual void RequestFriend(const FText& FriendName) = 0;

	/**
	* Request a friend to be deleted.
	*
	* @param FriendItem The friend item to delete.
	* @param DeleteReason The reason the friend is being deleted.
	*/
	virtual void DeleteFriend(TSharedPtr< IFriendItem > FriendItem, EFriendActionType::Type DeleteReason) = 0;

	/** 
	 * Create a new UniqueNetID 
	 */
	virtual TSharedPtr<const FUniqueNetId> CreateUniquePlayerId(const FString InUserID) = 0;

	/**
	* Find a recent player.
	*
	* @param InUserId The user id to find.
	* @return The recent player ID.
	*/
	virtual TSharedPtr< IFriendItem > FindRecentPlayer(const FUniqueNetId& InUserID) = 0;

	/**
	* Add a namespace to use when querying for recent players
	*/
	virtual void AddRecentPlayerNamespace(const FString& Namespace) = 0;

	/**
	* Get Current users nickname
	*/
	virtual FString GetUserNickname() const = 0;

	/**
	* Get client ID of current app
	*/
	virtual FString GetUserClientId() const = 0;

	/**
	* Get Current users online status
	*/
	virtual EOnlinePresenceState::Type GetOnlineStatus() = 0;

	/**
	* Set Current users online status
	* @param OnlineState The status
	*/
	virtual void SetUserIsOnline(EOnlinePresenceState::Type OnlineState) = 0;

	/**
	* Check if friend is in out session
	* @param FriendItem Friend to compare with
	* @return True if in same session
	*/
	virtual bool IsFriendInSameSession(const TSharedPtr< const IFriendItem >& FriendItem) const = 0;

	/**
	* Check if friend is in out party
	* @param FriendItem Friend to compare with
	* @return True if in same party
	*/
	virtual bool IsFriendInSameParty(const TSharedPtr< const IFriendItem >& FriendItem) const = 0;

	/**
	* Get party info our friend is in
	* @param FriendItem Friend to get party from
	* @return Party Info
	*/
	virtual TSharedPtr<IOnlinePartyJoinInfo> GetPartyJoinInfo(const TSharedPtr<const IFriendItem>& FriendItem) const = 0;

	/**
	* Find a user.
	*
	* @param InUserName The user name to find.
	* @return The Friend ID.
	*/
	virtual TSharedPtr< IFriendItem > FindUser(const FUniqueNetId& InUserID) = 0;
	virtual TSharedPtr< IFriendItem > FindUser(const TSharedRef<const FUniqueNetId>& InUserID) = 0;

	// Event declarations
	DECLARE_EVENT(FFriendsService, FOnFriendsUpdated)
	virtual FOnFriendsUpdated& OnFriendsListUpdated() = 0;

	DECLARE_EVENT_OneParam(FFriendsService, FOnAddToast, const FText)
	virtual FOnAddToast& OnAddToast() = 0;
};

/**
* Creates the implementation of a chat manager.
*
* @return the newly created FriendViewModel implementation.
*/
FACTORY(TSharedRef< FFriendsService >, FFriendsService, const TSharedRef<class FOSSScheduler>& OSSScheduler, const TSharedRef<class FChatNotificationService>& NotificationService);
