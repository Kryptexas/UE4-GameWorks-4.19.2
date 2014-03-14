// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineSubsystemTypes.h"

/** List of known friends list types */
namespace EFriendsLists
{
	enum Type
	{
		/** default friends list */
		Default,
		/** recent players friends list */
		RecentPlayers
	};

	/** @return the stringified version of the enum passed in */
	inline const TCHAR* ToString(EFriendsLists::Type EnumVal)
	{
		switch (EnumVal)
		{
			case Default:
				return TEXT("default");
			case RecentPlayers:
				return TEXT("recentPlayers");
		}
		return TEXT("");
	}
}

/**
 * Delegate used in friends list change notifications
 */
DECLARE_MULTICAST_DELEGATE(FOnFriendsChange);
typedef FOnFriendsChange::FDelegate FOnFriendsChangeDelegate;

/**
 * Delegate used when the friends read request has completed
 *
 * @param LocalUserNum the controller number of the associated user that made the request
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 * @param ListName name of the friends list that was operated on
 * @param ErrorStr string representing the error condition
 */
DECLARE_MULTICAST_DELEGATE_FourParams(FOnReadFriendsListComplete, int32, bool, const FString&, const FString&);
typedef FOnReadFriendsListComplete::FDelegate FOnReadFriendsListCompleteDelegate;

/**
 * Delegate used when the friends list delete request has completed
 *
 * @param LocalUserNum the controller number of the associated user that made the request
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 * @param ListName name of the friends list that was operated on
 * @param ErrorStr string representing the error condition
 */
DECLARE_MULTICAST_DELEGATE_FourParams(FOnDeleteFriendsListComplete, int32, bool, const FString&, const FString&);
typedef FOnDeleteFriendsListComplete::FDelegate FOnDeleteFriendsListCompleteDelegate;

/**
 * Delegate used when an invite send request has completed
 *
 * @param LocalUserNum the controller number of the associated user that made the request
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 * @param FriendId player that was invited
 * @param ListName name of the friends list that was operated on
 * @param ErrorStr string representing the error condition
 */
DECLARE_MULTICAST_DELEGATE_FiveParams(FOnSendInviteComplete, int32, bool, const FUniqueNetId&, const FString&, const FString&);
typedef FOnSendInviteComplete::FDelegate FOnSendInviteCompleteDelegate;

/**
 * Delegate used when an invite accept request has completed
 *
 * @param LocalUserNum the controller number of the associated user that made the request
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 * @param FriendId player that invited us
 * @param ListName name of the friends list that was operated on
 * @param ErrorStr string representing the error condition
 */
DECLARE_MULTICAST_DELEGATE_FiveParams(FOnAcceptInviteComplete, int32, bool, const FUniqueNetId&, const FString&, const FString&);
typedef FOnAcceptInviteComplete::FDelegate FOnAcceptInviteCompleteDelegate;

/**
 * Delegate used when an invite reject request has completed
 *
 * @param LocalUserNum the controller number of the associated user that made the request
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 * @param FriendId player that invited us
 * @param ListName name of the friends list that was operated on
 * @param ErrorStr string representing the error condition
 */
DECLARE_MULTICAST_DELEGATE_FiveParams(FOnRejectInviteComplete, int32, bool, const FUniqueNetId&, const FString&, const FString&);
typedef FOnRejectInviteComplete::FDelegate FOnRejectInviteCompleteDelegate;

/**
 * Delegate used when an friend delete request has completed
 *
 * @param LocalUserNum the controller number of the associated user that made the request
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 * @param FriendId player that was deleted
 * @param ListName name of the friends list that was operated on
 * @param ErrorStr string representing the error condition
 */
DECLARE_MULTICAST_DELEGATE_FiveParams(FOnDeleteFriendComplete, int32, bool, const FUniqueNetId&, const FString&, const FString&);
typedef FOnDeleteFriendComplete::FDelegate FOnDeleteFriendCompleteDelegate;

/**
 * Interface definition for the online services friends services 
 * Friends services are anything related to the maintenance of friends and friends lists
 */
class IOnlineFriends
{
protected:
	IOnlineFriends() {};

public:
	virtual ~IOnlineFriends() {};

	/**
     * Delegate used in friends list change notifications
     */
	DEFINE_ONLINE_PLAYER_DELEGATE(MAX_LOCAL_PLAYERS, OnFriendsChange);

	/**
	 * Starts an async task that reads the named friends list for the player 
	 *
	 * @param LocalUserNum the user to read the friends list of
	 * @param ListName name of the friends list to read
	 *
	 * @return true if the read request was started successfully, false otherwise
	 */
	virtual bool ReadFriendsList(int32 LocalUserNum, const FString& ListName) = 0;

	/**
	 * Delegate used when the friends list read request has completed
	 *
	 * @param LocalUserNum the controller number of the associated user that made the request
	 * @param bWasSuccessful true if the async action completed without error, false if there was an error
	 * @param ListName name of the friends list that was operated on
	 * @param ErrorStr string representing the error condition
	 */
	DEFINE_ONLINE_PLAYER_DELEGATE_THREE_PARAM(MAX_LOCAL_PLAYERS, OnReadFriendsListComplete, bool, const FString&, const FString&);

	/**
	 * Starts an async task that deletes the named friends list for the player 
	 *
	 * @param LocalUserNum the user to delete the friends list for
	 * @param ListName name of the friends list to delete
	 *
	 * @return true if the delete request was started successfully, false otherwise
	 */
	virtual bool DeleteFriendsList(int32 LocalUserNum, const FString& ListName) = 0;

	/**
     * Delegate used when the friends list delete request has completed
     *
	 * @param LocalUserNum the controller number of the associated user that made the request
     * @param bWasSuccessful true if the async action completed without error, false if there was an error
	 * @param ListName name of the friends list that was operated on
	 * @param ErrorStr string representing the error condition
     */
	DEFINE_ONLINE_PLAYER_DELEGATE_THREE_PARAM(MAX_LOCAL_PLAYERS, OnDeleteFriendsListComplete, bool, const FString&, const FString&);

	/**
	 * Starts an async task that sends an invite to another player. 
	 *
	 * @param LocalUserNum the user that is sending the invite
	 * @param FriendId player that is receiving the invite
	 * @param ListName name of the friends list to invite to
	 *
	 * @return true if the request was started successfully, false otherwise
	 */
	virtual bool SendInvite(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName) = 0;

	/**
	 * Delegate used when an invite send request has completed
	 *
	 * @param LocalUserNum the controller number of the associated user that made the request
	 * @param bWasSuccessful true if the async action completed without error, false if there was an error
	 * @param FriendId player that was invited
	 * @param ListName name of the friends list that was operated on
	 * @param ErrorStr string representing the error condition
	 */
	DEFINE_ONLINE_PLAYER_DELEGATE_FOUR_PARAM(MAX_LOCAL_PLAYERS, OnSendInviteComplete, bool, const FUniqueNetId&, const FString&, const FString&);

	/**
	 * Starts an async task that accepts an invite from another player. 
	 *
	 * @param LocalUserNum the user that is accepting the invite
	 * @param FriendId player that had sent the pending invite
	 * @param ListName name of the friends list to operate on
	 *
	 * @return true if the request was started successfully, false otherwise
	 */
	virtual bool AcceptInvite(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName) = 0;

	/**
	 * Delegate used when an invite accept request has completed
	 *
	 * @param LocalUserNum the controller number of the associated user that made the request
	 * @param bWasSuccessful true if the async action completed without error, false if there was an error
	 * @param FriendId player that invited us
	 * @param ListName name of the friends list that was operated on
	 * @param ErrorStr string representing the error condition
	 */
	DEFINE_ONLINE_PLAYER_DELEGATE_FOUR_PARAM(MAX_LOCAL_PLAYERS, OnAcceptInviteComplete, bool, const FUniqueNetId&, const FString&, const FString&);

	/**
	 * Starts an async task that rejects an invite from another player. 
	 *
	 * @param LocalUserNum the user that is rejecting the invite
	 * @param FriendId player that had sent the pending invite
	 * @param ListName name of the friends list to operate on
	 *
	 * @return true if the request was started successfully, false otherwise
	 */
 	virtual bool RejectInvite(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName) = 0;

	/**
	 * Delegate used when an invite reject request has completed
	 *
	 * @param LocalUserNum the controller number of the associated user that made the request
	 * @param bWasSuccessful true if the async action completed without error, false if there was an error
	 * @param FriendId player that invited us
	 * @param ListName name of the friends list that was operated on
	 * @param ErrorStr string representing the error condition
	 */
	DEFINE_ONLINE_PLAYER_DELEGATE_FOUR_PARAM(MAX_LOCAL_PLAYERS, OnRejectInviteComplete, bool, const FUniqueNetId&, const FString&, const FString&);

	/**
	 * Starts an async task that deletes a friend from the named friends list
	 *
	 * @param LocalUserNum the user that is making the request
	 * @param FriendId player that will be deleted
	 * @param ListName name of the friends list to operate on
	 *
	 * @return true if the request was started successfully, false otherwise
	 */
 	virtual bool DeleteFriend(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName) = 0;

	/**
	 * Delegate used when an friend delete request has completed
	 *
	 * @param LocalUserNum the controller number of the associated user that made the request
	 * @param bWasSuccessful true if the async action completed without error, false if there was an error
	 * @param FriendId player that was deleted
	 * @param ListName name of the friends list that was operated on
	 * @param ErrorStr string representing the error condition
	 */
	DEFINE_ONLINE_PLAYER_DELEGATE_FOUR_PARAM(MAX_LOCAL_PLAYERS, OnDeleteFriendComplete, bool, const FUniqueNetId&, const FString&, const FString&);

	/**
	 * Copies the list of friends for the player previously retrieved from the online service
	 *
	 * @param LocalUserNum the user to read the friends list of
	 * @param ListName name of the friends list to read
	 * @param OutFriends [out] array that receives the copied data
	 *
	 * @return true if friends list was found
	 */
	virtual bool GetFriendsList(int32 LocalUserNum, const FString& ListName, TArray< TSharedRef<FOnlineFriend> >& OutFriends) = 0;

	/**
	 * Get the cached friend entry if found
	 *
	 * @param LocalUserNum the user to read the friends list of
	 * @param ListName name of the friends list to read
	 * @param OutFriends [out] array that receives the copied data
	 *
	 * @return null ptr if not found
	 */
	virtual TSharedPtr<FOnlineFriend> GetFriend(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName) = 0;
 	
	/**
	 * Checks that a unique player id is part of the specified user's friends list
	 *
	 * @param LocalUserNum the controller number of the associated user that made the request
	 * @param FriendId the id of the player being checked for friendship
	 * @param ListName name of the friends list to read
	 *
	 * @return true if friends list was found and the friend was valid
	 */
	virtual bool IsFriend(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName) = 0;
};

typedef TSharedPtr<IOnlineFriends, ESPMode::ThreadSafe> IOnlineFriendsPtr;

