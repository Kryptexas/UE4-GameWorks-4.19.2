// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	FriendsMessageManager.h: Declares the FFriendsMessageManager interface.
=============================================================================*/

#pragma once

// Enum holding the state of the Friends manager
namespace EFriendsMessageManagerState
{
	enum Type
	{
		Idle,					// Manager is idle
		EnumeratingMessages,	// Enumerating messages
		ReadingMessages,		// Reading messages
		DeletingMessages,		// Deleting read messages
		SendingMessages,		// Sending messages
		SendingGameInvite,		// Sending game invite messages
		SendingJoinGameRequest,	// Sending join game messages
	};
};

/**
 * Delegate type for FriendsList updated.
 */
DECLARE_MULTICAST_DELEGATE( FOnMessagesUpdated )

/**
 * Implement the Friend and Chat manager
 */
class FFriendsMessageManager
	: public TSharedFromThis<FFriendsMessageManager>
{
	// Stuct for outgoing chat messages
	struct FOutGoingChatMessage
	{
		FOutGoingChatMessage( TSharedRef< FUniqueNetId > InFriendID,  const FText& InMessage )
			: FriendID( InFriendID )
			, Message( InMessage )
		{}

		// Holds the Friend net ID
		TSharedRef< FUniqueNetId > FriendID;
		// Holds the message to send
		const FText& Message;
	};


public:

	/**
	 * Default constructor
	 */
	FFriendsMessageManager();
	
	/**
	 * Default destructor
	 */
	~FFriendsMessageManager();

public:

	/**
	 * Default constructor
	 */
	void StartupManager();

	/**
	 * Init the manager
	 * @param NotificationDelegate 	- the notification delegate
	 * @param bInAllowLaunchGame 	- Can this app join a game directly
	 */
	void Init( FOnFriendsNotification& NotificationDelegate, bool bInAllowLaunchGame );

	/**
	 * Start or stop message polling for messages
	 * @param bStart - true for start - false for stop;
	 */
	void SetMessagePolling( bool bStart );

	/**
	 * Logout - close any Friends windows.
	 */
	void Logout();

	/**
	 * Request a friend to be invited to a game
	 * @param FriendID - The friend ID
	 */
	void InviteFriendToGame( TSharedRef< FUniqueNetId > FriendID );

	/**
	 * Request to join a friends game
	 * @param FriendID - The friend ID
	 */
	void RequestJoinAGame( TSharedRef< FUniqueNetId > FriendID );

	/**
	 * Send a chat message
	 * @param FriendID - Friend to send a message to.
 	 * @param Message - The message content;
	 */
	void SendMessage( TSharedPtr< FFriendStuct > FriendID, const FText& Message );

	/**
	 * Clear game invites
	 */
	void ClearGameInvites();
	
	/**
	 * Set an unhandled notification. We will try again later
	 * @param NetID - The NetID of the failed notfication
	 */
	void SetUnhandledNotification( TSharedRef< FUniqueNetId > NetID );

	/**
	 * Get the manager state
	 * @return - The manager state
	 */
	EFriendsMessageManagerState::Type GetState();

	/**
	 * Get current chat messages
	 * @return - A list of chat messages
	 */
	TArray< TSharedPtr< FFriendsAndChatMessage > > GetChatMessages();

	/**
	 * Accessor for the Friends List updated delegate.
	 * @return The delegate
	 */
	FOnMessagesUpdated& OnChatListUpdated();

private:

	/**
	 * A ticker used to perform updates on the main thread.
	 * @param Delta		The tick delta
	 * @return	true to continue ticking
	 */
	bool Tick( float Delta );

	/**
	 * Set the manager state
	 * @param NewState - The new manager state
	 */
	void SetState( EFriendsMessageManagerState::Type NewState );

	/**
	 * Enumerate messages.
	 */
	void RequestEnumerateMessages();

	/**
	 * Send a game invite request
	 */
	void SendGameInviteRequest();

	/**
	 * Send join game request
	 */
	void SendGameJoinRequest();

	/**
	 * Send a chat message.
	 */
	void SendChatMessageRequest();

	/**
	 * Delegate used when enumeration of messages is complete
	 *
	 * @param LocalPlayer		- the controller number of the associated user that made the request
	 * @param bWasSuccessful	- true if the async action completed without error, false if there was an error
	 * @param ErrorStr			- string representing the error condition
	 */
	void OnEnumerateMessagesComplete(int32 LocalPlayer, bool bWasSuccessful, const FString& ErrorStr);

	/**
	 * Delegate used when a read message is complete
	 *
	 * @param LocalPlayer		- the controller number of the associated user that made the request
	 * @param bWasSuccessful	- true if the async action completed without error, false if there was an error
	 * @param MessageId			- the message ID
	 * @param ErrorStr			- string representing the error condition
	 */
	void OnReadMessageComplete(int32 LocalPlayer, bool bWasSuccessful, const FUniqueMessageId& MessageId, const FString& ErrorStr);

	/**
	 * Delegate used when an send message has completed
	 *
	 * @param LocalPlayer		- the controller number of the associated user that made the request
	 * @param bWasSuccessful	- true if the async action completed without error, false if there was an error
	 * @param ErrorStr			- string representing the error condition
	 */
	void OnSendMessageComplete(int32 LocalPlayer, bool bWasSuccessful, const FString& ErrorStr);

	/**
	 * Delegate used when a delete message has completed
	 *
	 * @param LocalPlayer		- the controller number of the associated user that made the request
	 * @param bWasSuccessful	- true if the async action completed without error, false if there was an error
	 * @param MessageId			- the message ID
	 * @param ErrorStr			- string representing the error condition
	 */
	void OnDeleteMessageComplete(int32 LocalPlayer, bool bWasSuccessful, const FUniqueMessageId& MessageId, const FString& ErrorStr);

	/**
	 * Handle an accept message accepted from a notification
	 *
	 * @param MessageNotification 	- The message responded to
	 */
	FReply HandleMessageAccepted( TSharedPtr< FFriendsAndChatMessage > MessageNotification );

	/**
	 * Handle an accept message accepted from a notification
	 *
	 * @param MessageNotification 	- The message responded to
	 */
	FReply HandleMessageDeclined( TSharedPtr< FFriendsAndChatMessage > MessageNotification );

	/**
	 * Resend a failed notification message.
	 */
	void ResendMessage();

private:

	// Delegate to use for enumerating messages for a user
	FOnEnumerateMessagesCompleteDelegate OnEnumerateMessagesCompleteDelegate;
	// Delegate to use for downloading messages for a user
	FOnReadMessageCompleteDelegate OnReadMessageCompleteDelegate;
	// Delegate to use for sending messages for a user
	FOnSendMessageCompleteDelegate OnSendMessageCompleteDelegate;
	// Delegate to use for deleting messages for a user
	FOnDeleteMessageCompleteDelegate OnDeleteMessageCompleteDelegate;
	// Holds the delegate to call when the friends list gets updated - refresh the UI
	FOnMessagesUpdated OChatListUpdatedDelegate;
	// Holds the array of outgoing chat messages
	TArray< FOutGoingChatMessage > ChatMessagesToSend;

	// Holds array of outgoing Game Invites.
	TArray< TSharedRef< FUniqueNetId > > GameInvitesToSend;
	// Holds array of outgoing Join Game requests.
	TArray< TSharedRef< FUniqueNetId > > GameJoingRequestsToSend;
	// Holds list of messages to download
	TArray<TSharedRef< FUniqueMessageId > > MessagesToRead;
	// Holds list of messages to delete
	TArray< FUniqueMessageId* > MessagesToDelete;
	// Holds list of incoming chat messages
	TArray<TSharedPtr< FFriendsAndChatMessage > > ChatMessages;
	// Holds list of incoming notifications messages
	TArray<TSharedPtr< FFriendsAndChatMessage > > NotficationMessages;
	// Holds a message that will be responded to later on
	FString LatentMessage;
	// Holds a message that should be acted on
	FString GameLaunchMessageID;
	// Holds if can join a game;
	bool bCanJoinGame;
	// Holds should clear invites;
	bool bClearInvites;
	// Holds the manager state
	EFriendsMessageManagerState::Type ManagerState;
	// Holds the unhandled notification Net ID to resend
	TSharedPtr< FUniqueNetId > UnhandledNetID;

	// current time in seconds remaining before pinging mcp services again
	float PingMcpCountdown;
	// interval in seconds before pinging mcp services
	const float PingMcpInterval;
	// Holds the online subsystem MCP
	FOnlineSubsystemMcp* OnlineSubMcp;
	// Holds if we should be polling for messages
	bool bPollForMessages;

	// Holds the user display name
	FString DisplayName;
	// Holds the ticker delegate
	FTickerDelegate UpdateMessagesTickerDelegate;
	// Holds the notification delegate
	FOnFriendsNotification* FriendsListNotificationDelegate;

/* Here we have static access for the singleton
*****************************************************************************/
public:
	static TSharedRef< FFriendsMessageManager > Get();
	static void Shutdown();
private:
	static TSharedPtr< FFriendsMessageManager > SingletonInstance;
};
