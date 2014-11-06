// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


// Enum holding the state of the Friends manager
namespace EFriendsAndManagerState
{
	enum Type
	{
		Idle,								// Idle - can accept requests
		RequestingFriendsList,				// Requesting a list refresh
		RequestFriendsListRefresh,			// List request in progress
		ProcessFriendsList,					// Process the Friends List after a list refresh
		RequestingFriendName,				// Requesting a friend add
		DeletingFriends,					// Deleting a friend
		AcceptingFriendRequest,				// Accepting a friend request
		OffLine,							// No logged in
	};
};

/**
 * Implement the Friend and Chat manager
 */
class FFriendsAndChatManager
	: public IFriendsAndChatManager
	, public TSharedFromThis<FFriendsAndChatManager>
{
public:

	/** Default constructor. */
	FFriendsAndChatManager( );
	
	/** Destructor. */
	~FFriendsAndChatManager( );

public:

	// IFriendsAndChatManager
	virtual void Logout() override;
	virtual void Login() override;
	virtual void CreateFriendsListWidget( TSharedPtr< const SWidget > ParentWidget, const FFriendsAndChatStyle* InStyle ) override;
	virtual void SetUserSettings(FFriendsAndChatSettings UserSettings) override;
	virtual TSharedPtr< SWidget > GenerateFriendsListWidget( const FFriendsAndChatStyle* InStyle ) override;
	virtual TSharedPtr< SWidget > GenerateChatWidget( const FFriendsAndChatStyle* InStyle ) override;
	virtual void SetInSession( bool bInSession ) override;
	virtual void InsertNetworkChatMessage(const FString InMessage) override;

	/**
	 * Get if the player is in a session.
	 *
	 * @return True if we are in a game session.
	 */
	bool IsInSession();

	/**
	 * Create the friends list window.
	 *
	 * @param FriendItem The friend to start a chat with.
	 */
	void GenerateChatWindow( TSharedPtr< FFriendStuct > FriendItem );

	/**
	 * Accept a friend request.
	 *
	 * @param FriendItem The friend to accept.
	 */
	void AcceptFriend( TSharedPtr< FFriendStuct > FriendItem );

	/**
	 * Reject a friend request.
	 *
	 * @param FriendItem The friend to reject.
	 */
	void RejectFriend( TSharedPtr< FFriendStuct > FriendItem );

	/**
	 * Get the friends filtered list of friends.
	 *
	 * @param OutFriendsList  Array of friends to fill in.
	 * @return the friend list count.
	 */
	int32 GetFilteredFriendsList( TArray< TSharedPtr< FFriendStuct > >& OutFriendsList );

	/**
	 * Get outgoing request list.
	 *
	 * @param OutFriendsList  Array of friends to fill in.
	 * @return The friend list count.
	 */
	int32 GetFilteredOutgoingFriendsList( TArray< TSharedPtr< FFriendStuct > >& OutFriendsList );

	/**
	 * Request a friend be added.
	 *
	 * @param FriendName The friend name.
	 */
	void RequestFriend( const FText& FriendName );

	/**
	 * Request a friend to be deleted.
	 *
	 * @param FriendItem The friend item to delete.
	 */
	void DeleteFriend( TSharedPtr< FFriendStuct > FriendItem );

	/**
	 * Find a user ID.
	 *
	 * @param InUserName The user name to find.
	 * @return The unique ID.
	 */
	TSharedPtr< FUniqueNetId > FindUserID( const FString& InUsername );

	/**
	 * Is the owner online
	 *
	 * @return true if online
	 */
	bool GetUserIsOnline();

	/**
	 * Set the user online status
	 *
	 * @param bIsOnline - the online state
	 */
	void SetUserIsOnline(bool bIsOnline);

	/**
	 * Find a user.
	 *
	 * @param InUserName The user name to find.
	 * @return The Friend ID.
	 */
	TSharedPtr< FFriendStuct > FindUser(const FUniqueNetId& InUserID);

	// External events
	DECLARE_DERIVED_EVENT(FFriendsAndChatManager, IFriendsAndChatManager::FOnFriendsNotificationEvent, FOnFriendsNotificationEvent)
	virtual FOnFriendsNotificationEvent& OnFriendsNotification() override
	{
		return FriendsListNotificationDelegate;
	}

	DECLARE_DERIVED_EVENT(FFriendsAndChatManager, IFriendsAndChatManager::FOnFriendsNotificationActionEvent, FOnFriendsNotificationActionEvent)
	virtual FOnFriendsNotificationActionEvent& OnFriendsActionNotification() override
	{
		return FriendsListActionNotificationDelegate;
	}

	DECLARE_DERIVED_EVENT(IFriendsAndChatManager, IFriendsAndChatManager::FOnFriendsUserSettingsUpdatedEvent, FOnFriendsUserSettingsUpdatedEvent)
	virtual FOnFriendsUserSettingsUpdatedEvent& OnFriendsUserSettingsUpdated() override
	{
		return FriendsUserSettingsUpdatedDelegate;
	}

	DECLARE_DERIVED_EVENT(IFriendsAndChatManager, IFriendsAndChatManager::FOnFriendsSendNetworkMessageEvent, FOnFriendsSendNetworkMessageEvent)
	virtual FOnFriendsSendNetworkMessageEvent& OnFriendsSendNewtworkMessage() override
	{
		return FriendsSendNetworkMessageEvent;
	}

	// Internal events

	DECLARE_EVENT(FFriendsAndChatManager, FOnFriendsUpdated)
	virtual FOnFriendsUpdated& OnFriendsListUpdated()
	{
		return OnFriendsListUpdatedDelegate;
	}

private:

	/** Process the friends list to send out invites */
	void SendFriendRequests();

	/**
	 * Request a list read
	 */
	void RequestListRefresh();

	/**
	 * Pre process the friends list - find missing names etc
	 *
	 * @param ListName - the list name
	 */
	void PreProcessList(const FString& ListName);

	/** Refresh the data lists */
	void RefreshList();

	/** Build the friends UI. */
	void BuildFriendsUI();

	/**
	 * Set the manager state.
	 *
	 * @param NewState The new manager state.
	 */
	void SetState( EFriendsAndManagerState::Type NewState );

	/** Send a friend invite notification. */
	void SendFriendInviteNotification();

	/** Send a friend invite accepted notification. */
	void SendInviteAcceptedNotification(TSharedPtr< FFriendStuct > Friend);

	/** Called when singleton is released. */
	void ShutdownManager();

	/**
	 * Delegate used when the friends read request has completed.
	 *
	 * @param LocalPlayer		The controller number of the associated user that made the request.
	 * @param bWasSuccessful	true if the async action completed without error, false if there was an error.
	 * @param ListName			Name of the friends list that was operated on.
	 * @param ErrorStr			String representing the error condition.
	 */
	void OnReadFriendsListComplete( int32 LocalPlayer, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr );

	/**
	 * Delegate used when an invite send request has completed.
	 *
	 * @param LocalPlayer		The controller number of the associated user that made the request.
	 * @param bWasSuccessful	true if the async action completed without error, false if there was an error.
	 * @param FriendId			Player that was invited.
	 * @param ListName			Name of the friends list that was operated on.
	 * @param ErrorStr			String representing the error condition.
	 */
	void OnSendInviteComplete( int32 LocalPlayer, bool bWasSuccessful, const FUniqueNetId& FriendId, const FString& ListName, const FString& ErrorStr );

	/**
	 * Delegate used when an invite accept request has completed.
	 *
	 * @param LocalPlayer		The controller number of the associated user that made the request
	 * @param bWasSuccessful	true if the async action completed without error, false if there was an error
	 * @param FriendId			Player that invited us
	 * @param ListName			Name of the friends list that was operated on
	 * @param ErrorStr			String representing the error condition
	 */
	void OnAcceptInviteComplete( int32 LocalPlayer, bool bWasSuccessful, const FUniqueNetId& FriendId, const FString& ListName, const FString& ErrorStr );

	/**
	 * Delegate used when the friends delete request has completed.
	 *
	 * @param LocalPlayer		The controller number of the associated user that made the request.
	 * @param bWasSuccessful	true if the async action completed without error, false if there was an error.
	 * @param DeletedFriendID	The ID of the deleted friend.
	 * @param ListName			Name of the friends list that was operated on.
	 * @param ErrorStr			String representing the error condition.
	 */
	void OnDeleteFriendComplete( int32 LocalPlayer, bool bWasSuccessful, const FUniqueNetId& DeletedFriendID, const FString& ListName, const FString& ErrorStr );

	/**
	 * Delegate used when a friend ID request has completed.
	 *
	 * @param bWasSuccessful		true if the async action completed without error, false if there was an error.
	 * @param RequestingUserId		The ID of the user making the request.
	 * @param DisplayName			The string ID of the friend found.
	 * @param IdentifiedUserId		The net ID of the found friend.
	 * @param ErrorStr				String representing the error condition.
	 */
	void OnQueryUserIdMappingComplete(bool bWasSuccessful, const FUniqueNetId& RequestingUserId, const FString& DisplayName, const FUniqueNetId& IdentifiedUserId, const FString& Error);

	/**
	 * Delegate used when a query user info request has completed.
	 *
	 * @param LocalPlayer		The controller number of the associated user that made the request.
	 * @param bWasSuccessful	true if the async action completed without error, false if there was an error.
	 * @param UserIds			An array of user IDs filled out with info.
	 * @param ErrorStr			String representing the error condition.
	 */
	void OnQueryUserInfoComplete( int32 LocalPlayer, bool bWasSuccessful, const TArray< TSharedRef<class FUniqueNetId> >& UserIds, const FString& ErrorStr );

	/**
	 * Delegate used when a query presence info request has completed.
	 *
	 * @param UserId		The user ID.
	 * @param Presence	The user presence.
	 */
	void OnPresenceReceived( const class FUniqueNetId& UserId, const TSharedRef<FOnlineUserPresence>& Presence);

	/**
	 * Delegate used when a users presence is updated.
	 *
	 * @param UserId		The user ID.
	 * @param Presence	The user presence.
	 */
	void OnPresenceUpdated(const class FUniqueNetId& UserId, const bool bWasSuccessful);

	/**
	 * Delegate called when the friends list changes.
	 */
	void OnFriendsListChanged();

	/**
	 * Delegate called when an invite is received.
	 *
	 * @param UserId		The user ID.
	 * @param FriendId	The friend ID.
	 */
	void OnFriendInviteReceived(const FUniqueNetId& UserId, const FUniqueNetId& FriendId);

	/**
	 * Delegate used when a friend is removed.
	 *
	 * @param UserId		The user ID.
	 * @param FriendId	The friend ID.
	 */
	void OnFriendRemoved(const FUniqueNetId& UserId, const FUniqueNetId& FriendId);

	/**
	 * Delegate used when an invite is rejected.
	 *
	 * @param UserId		The user ID.
	 * @param FriendId	The friend ID.
	 */
	void OnInviteRejected(const FUniqueNetId& UserId, const FUniqueNetId& FriendId);

	/**
	 * Delegate used when an invite is accepted.
	 *
	 * @param UserId		The user ID.
	 * @param FriendId	The friend ID.
	 */
	void OnInviteAccepted(const FUniqueNetId& UserId, const FUniqueNetId& FriendId);

	/**
	 * Handle an accept message accepted from a notification.
	 *
	 * @param MessageNotification The message responded to.
 	 * @param ResponseTpe The response from the message request.
	 */
	FReply HandleMessageAccepted( TSharedPtr< FFriendsAndChatMessage > MessageNotification, EFriendsResponseType::Type ResponseType );

	/**
	 * Validate friend list after a refresh. Check if the list has changed.
	 *
	 * @return True if the list has changes and the UI needs refreshing.
	 */
	bool ProcessFriendsList();

	/**
	 * A ticker used to perform updates on the main thread.
	 *
	 * @param Delta The tick delta.
	 * @return true to continue ticking.
	 */
	bool Tick( float Delta );

	/** Add a friend toast. */
	void AddFriendsToast(const FText Message);

private:

	/* Lists used to keep track of Friends
	*****************************************************************************/

	// Holds the list of user names to query add as friends 
	TArray< FString > FriendByNameRequests;
	// Holds the list of user names to send invites to. 
	TArray< FString > FriendByNameInvites;
	// Holds the list of Unique Ids found for user names to add as friends
	TArray< TSharedRef< FUniqueNetId > > QueryUserIds;
	// Holds the full friends list used to build the UI
	TArray< TSharedPtr< FFriendStuct > > FriendsList;
	// Holds the filtered friends list used in the UI
	TArray< TSharedPtr< FFriendStuct > > FilteredFriendsList;
	// Holds the outgoing friend request list used in the UI
	TArray< TSharedPtr< FFriendStuct > > FilteredOutgoingList;
	// Holds the unprocessed friends list generated from a friends request update
	TArray< TSharedPtr< FFriendStuct > > PendingFriendsList;
	// Holds the list of incoming invites that need to be responded to
	TArray< TSharedPtr< FFriendStuct > > PendingIncomingInvitesList;
	// Holds the list of invites we have already responded to
	TArray< TSharedPtr< FUniqueNetId > > NotifiedRequest;
	// Holds the list messages sent out to be responded to
	TArray<TSharedPtr< FFriendsAndChatMessage > > NotificationMessages;
	// Holds an array of outgoing invite friend requests
	TArray< TSharedRef< FUniqueNetId > > PendingOutgoingFriendRequests;
	// Holds an array of outgoing delete friend requests
	TArray< FUniqueNetIdString > PendingOutgoingDeleteFriendRequests;
	// Holds an array of outgoing accept friend requests
	TArray< FUniqueNetIdString > PendingOutgoingAcceptFriendRequests;

	/* Delegates
	*****************************************************************************/

	// Holds the ticker delegate
	FTickerDelegate UpdateFriendsTickerDelegate;
	// Delegate when friend list has been read
	FOnReadFriendsListCompleteDelegate OnReadFriendsCompleteDelegate;
	// Delegate when invites accepted
	FOnAcceptInviteCompleteDelegate OnAcceptInviteCompleteDelegate;
	// Delegate when invites have been sent
	FOnSendInviteCompleteDelegate OnSendInviteCompleteDelegate;
	// Delegate to use for deleting a friend
	FOnDeleteFriendCompleteDelegate OnDeleteFriendCompleteDelegate;
	// Delegate for querying user id from a name string
	FOnQueryUserIdMappingCompleteDelegate OnQueryUserIdMappingCompleteDelegate;
	// Delegate to use for querying user info list
	FOnQueryUserInfoCompleteDelegate OnQueryUserInfoCompleteDelegate;
	// Delegate to use for querying user presence
	FOnPresenceReceivedDelegate OnPresenceReceivedCompleteDelegate;
	// Delegate to owner presence updated
	IOnlinePresence::FOnPresenceTaskCompleteDelegate OnPresenceUpdatedCompleteDelegate;
	// Delegate for friends list changing
	FOnFriendsChangeDelegate OnFriendsListChangedDelegate;
	// Delegate for an invite received
	FOnInviteReceivedDelegate OnFriendInviteReceivedDelegate;
	// Delegate for friend removed
	FOnFriendRemovedDelegate OnFriendRemovedDelegate;
	// Delegate for friend invite rejected
	FOnInviteRejectedDelegate	OnFriendInviteRejected;
	// Delegate for friend invite accepted
	FOnInviteAcceptedDelegate	OnFriendInviteAccepted;

	// Holds the Friends list notification delegate
	FOnFriendsNotificationEvent FriendsListNotificationDelegate;
	// Holds the Friends Action notification delegate
	FOnFriendsNotificationActionEvent FriendsListActionNotificationDelegate;
	// Holds the Options Updated event notification delegate
	FOnFriendsUserSettingsUpdatedEvent FriendsUserSettingsUpdatedDelegate;
	// Holds the network chat message sent delegate
	FOnFriendsSendNetworkMessageEvent FriendsSendNetworkMessageEvent;

	// Internal events
	// Holds the delegate to call when the friends list gets updated - refresh the UI
	FOnFriendsUpdated OnFriendsListUpdatedDelegate;

	/* Identity stuff
	*****************************************************************************/

	// Holds the Online identity
	IOnlineIdentityPtr OnlineIdentity;
	// Holds the Friends Interface
	IOnlineFriendsPtr FriendsInterface;
	// Holds the Online Subsystem
	FOnlineSubsystemMcp* OnlineSubMcp;

	TSharedPtr<class FFriendsMessageManager> MessageManager;
	TSharedPtr<class FChatViewModel> ChatViewModel;

	/* Manger state
	*****************************************************************************/

	// Holds the manager state
	EFriendsAndManagerState::Type ManagerState;

	/* UI
	*****************************************************************************/
	// Holds the parent window
	TSharedPtr< const SWidget > ParentWidget;
	// Holds the main Friends List window
	TSharedPtr< SWindow > FriendWindow;
	// Holds the Friends List widget
	TSharedPtr< SWidget > FriendListWidget;
	// Holds the chat widget
	TSharedPtr< SWidget > ChatWidget;
	// Holds the chat window
	TSharedPtr< SWindow > ChatWindow;
	// Holds the style used to create the Friends List widget
	FFriendsAndChatStyle Style;
	// Holds if the game is in session
	bool bIsInSession;
	// Holds if the Friends list is inited
	bool bIsInited;
	// Holds the Friends system user settings
	FFriendsAndChatSettings UserSettings;
	bool bRequiresListRefresh;
	// Holds the toast notification
	TSharedPtr<SNotificationList> FriendsNotificationBox;

public:

	static TSharedRef< FFriendsAndChatManager > Get();
	static void Shutdown();

private:

	static TSharedPtr< FFriendsAndChatManager > SingletonInstance;
};
