// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
* Implement the Friends data access
*/
class FGameAndPartyService
	: public IGameAndPartyService
	, public TSharedFromThis<FGameAndPartyService>
{
public:

	/** Destructor. */
	virtual ~FGameAndPartyService() {};

	virtual void Login() = 0;
	virtual void Logout() = 0;


	/**
	* Get incoming game invite list.
	*
	* @param OutFriendsList  Array of friends to fill in.
	* @return The friend list count.
	*/
	virtual int32 GetFilteredGameInviteList(TArray< TSharedPtr< IFriendItem > >& OutFriendsList) const = 0;

	/**
	* Get the name of the primary party chat room we've joined
	*
	* @param Out shared ref to party room id
	*/
	virtual TSharedPtr<const FOnlinePartyId> GetPartyChatRoomId() const = 0;

	/**
	* Reject a game invite
	*
	* @param FriendItem friend with game invite info
	*/
	virtual void RejectGameInvite(const TSharedPtr<IFriendItem>& FriendItem) = 0;

	/**
	* Accept a game invite
	*
	* @param FriendItem friend with game invite info
	*/
	virtual void AcceptGameInvite(const TSharedPtr<IFriendItem>& FriendItem) = 0;

	/**
	* Send a game invite to a friend
	*
	* @param FriendItem friend to send game invite to
	*/
	virtual void SendGameInvite(const TSharedPtr<IFriendItem>& FriendItem) = 0;

	/**
	* Send a game invite to a user by id
	*
	* @param UserId user to send game invite to
	*/
	virtual void SendGameInvite(const FUniqueNetId& ToUser) = 0;

	/**
	* Send a party invite to a user by id
	*
	* @param UserId user to send party invite to
	*/
	virtual bool SendPartyInvite(const FUniqueNetId& ToUser) = 0;

	/**
	* Get if the current player is in a session.
	*
	* @return True if we are in a game session.
	*/
	virtual bool IsInGameSession() const = 0;

	/**
	* Is this friend in the same session as I am
	*
	* @param reference to a friend item
	*/
	virtual bool IsFriendInSameSession(const TSharedPtr< const IFriendItem >& FriendItem) const = 0;

	/**
	* Is this friend in the same party as I am
	*
	* @param reference to a friend item
	*/
	virtual bool IsFriendInSameParty(const TSharedPtr< const IFriendItem >& FriendItem) const = 0;

	/**
	* Get if the current player is in a party and is joinable.
	*
	* @return true if the local user considers their party joinable, otherwise false
	*/
	virtual bool IsInJoinableParty() const = 0;

	/**
	* Get if the current player is in a session and that game is joinable.
	* The context here is from the local perspective, it should not be interpreted by or given to external recipients
	* (ie, the game is invite only and therefore joinable via invite, but it is not "joinable" from another user's perspective)
	*
	* @return true if the local user considers their game joinable, otherwise false
	*/
	virtual bool IsInJoinableGameSession() const = 0;

	/**
	* @param ClientID ID of the Game we want to join
	* @return true if joining a game is allowed
	*/
	virtual bool JoinGameAllowed(FString ClientID) = 0;

	/**
	* Get party info our friend is in
	* @param FriendItem Friend to get party from
	* @return Party Info
	*/
	virtual TSharedPtr<IOnlinePartyJoinInfo> GetPartyJoinInfo(const TSharedPtr<const IFriendItem>& FriendItem) const = 0;

	/**
	* Get Unique ID for our game session
	* @return session ID
	*/
	virtual TSharedPtr<const FUniqueNetId> GetGameSessionId() const = 0;

	/**
	* Get Unique ID for this session string
	* @param SessionID the ID in string format
	* @return session ID
	*/
	virtual TSharedPtr<const FUniqueNetId> GetGameSessionId(FString SessionID) const = 0;

	/**
	* Get if we are in a party
	* @return true if we are in a party
	*/
	virtual bool IsInActiveParty() const = 0;

	/**
	* Add an application view model
	* @param ClientID the ID of the app
	* @param InApplicationViewModel the view model of the app
	*/
	virtual void AddApplicationViewModel(const FString ClientID, TSharedPtr<IFriendsApplicationViewModel> InApplicationViewModel) = 0;

	/**
	* Clear list of application view models
	*/
	virtual void ClearApplicationViewModels() = 0;

	/**
	* Get our clientID
	* @return Client ID in string format
	*/
	virtual FString GetUserClientId() const = 0;

	/**
	* @return true if in the launcher
	*/
	virtual bool IsInLauncher() const = 0;

	DECLARE_EVENT(FGameAndPartyService, FOnGameInvitesUpdated)
	virtual FOnGameInvitesUpdated& OnGameInvitesUpdated() = 0;

	DECLARE_EVENT_TwoParams(FGameAndPartyService, FOnFriendsJoinGameEvent, const FUniqueNetId& /*FriendId*/, const FUniqueNetId& /*SessionId*/)
	virtual FOnFriendsJoinGameEvent& OnFriendsJoinGame() = 0;

};

/**
* Creates the implementation of a chat manager.
*
* @return the newly created FriendViewModel implementation.
*/
FACTORY(TSharedRef< FGameAndPartyService >, FGameAndPartyService, const TSharedRef<class FOSSScheduler>& OSSScheduler, const TSharedRef<class FFriendsService>& FriendsService, const TSharedRef<class FChatNotificationService>& NotificationService, bool bIsInGame);
