// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	FriendAndChatMessage.h: Declares the FriendsAndChatMessage class.
=============================================================================*/

#pragma once

/**
 * Type definition for shared references to instances of ILauncherProfile.
 */
typedef TSharedRef<class FFriendsAndChatMessage> FFriendsAndChatMessageRef;

// Enum to list download status flags
namespace EFriendsRequestType
{
	enum Type
	{
		// A chat message
		ChatMessage,
		// Request to join a game
		JoinGame,
		// Invite someone to join your game
		GameInvite,
		// Invite someone to be a friend
		FriendInvite,
		// The message is handled. Clear the notification if present
		MessageHandled,
	};
};

// Enum to list friend respose type
namespace EFriendsResponseType
{
	enum Type
	{
		// Accept a request
		Response_Accept,
		// Reject a request
		Response_Reject,
		// Ignore a request
		Response_Ignore,
	};
};

// Enum holding the display list selection
namespace EFriendsDisplayLists
{
	enum Type
	{
		DefaultDisplay, 		// Default friend list display
		RecentPlayersDisplay,	// Recent Players display
		FriendRequestsDisplay,	// Friend request display
	};
};

// Class containing the friend message information
class FFriendsAndChatMessage
{
public:
	/**
	 * Constructor.
	 * @param InMessage - The message content.
	 */
	FFriendsAndChatMessage( const FString& InMessage )
		: MessageConent( InMessage )
		, bAutoAccept( false )
	{}

	/**
	 * Constructor.
	 * @param InMessage			- The message content.
	 * @param InUniqueFriendID	- The Friend ID.
	 */
	FFriendsAndChatMessage( const FString& InMessage, const TSharedRef< FUniqueNetId > InUniqueFriendID )
		: MessageConent( InMessage )
		, UniqueFriendID( InUniqueFriendID )
		, bAutoAccept( false )
	{}

	/**
	 * Get the message content.
	 * @return The message.
	 */
	FString GetMessage()
	{
		return MessageConent;
	}

	/**
	 * Get the Net ID for the Friend this message is sent to.
	 * @return The net ID.
	 */
	const TSharedRef< FUniqueNetId > GetUniqueID()
	{
		return UniqueFriendID.ToSharedRef();
	}

	/**
	 * Set a button callbacks.
	 * @param InCallback - The button callback.
	 */
	void SetButtonCallback( FOnClicked InCallback )
	{
		ButtonCallbacks.Add( InCallback );
	}

	/**
	 * Set the message type.
	 * @param InType - The message type.
	 */
	void SetMessageType( EFriendsRequestType::Type InType )
	{
		MessageType = InType;
	}

	/**
	 * Sets the ID to use when joinging a game from an invite. Passed through the commandline.
	 * @param MessageID - The string representation of the NetID.
	 */
	void SetLaunchGameID( const FString& MessageID )
	{
		LaunchGameID = MessageID;
	}
	
	/**
	 * Gets the ID to use when joinging a game from an invite. Passed through the commandline.
	 * @return string representation of the message ID.
	 */
	const FString& GetLauchGameID() const
	{
		return LaunchGameID;
	}

	/**
	 * Get the message type.
	 * @return The message type.
	 */
	EFriendsRequestType::Type GetMessageType()
	{
		return MessageType;
	}

	/**
	 * Set the requester name.
	 * @param InName - The requesters name.
	 */
	void SetRequesterName( const FString& InName )
	{
		RequesterName = InName;
	}

	/**
	 * Get the requester name.
	 * @return - The requesters name.
	 */
	const FString& GetRequesterName() const
	{
		return RequesterName;
	}

	/**
	 * Set if this message should be handled by the caller, not the Friends Module.
	 * @param bInSelfHandle - True if the caller handles this message themselves.
	 */
	void SetSelfHandle( bool bInSelfHandle )
	{
		bSelfHandle = bInSelfHandle;
	}

	/**
	 * Set if this message should accepted automatically.
	 */
	void SetAutoAccept()
	{
		bAutoAccept = true;
	}

	/**
	 * Should the caller auto accept this message.
	 * @return True if the user should handle auto accept this message.
	 */
	const bool IsAutoAccepted() const
	{
		return bAutoAccept;
	}

	/**
	 * Should the caller handle this message themselves.
	 * @return True if the user should handle the message themselves.
	 */
	const bool IsSelfHandle() const
	{
		return bSelfHandle;
	}

	/**
	 * Get a button callbacks.
	 * @return The button callback array.
	 */
	TArray< FOnClicked > GetCallbacks()
	{
		return ButtonCallbacks;
	}

	/**
	 * Set this message into a handled state.
	 */
	void SetHandled()
	{
		MessageType = EFriendsRequestType::MessageHandled;
	}

private:
	// Holds the button callbacks
	TArray< FOnClicked > ButtonCallbacks;
	// Holds the message content
	FString MessageConent;
	// Holds the Unique Friend Net ID
	const TSharedPtr< FUniqueNetId > UniqueFriendID;
	// Holds the message type
	EFriendsRequestType::Type MessageType;
	// Holds the requester name.
	FString RequesterName;
	// Holds if the message should be self handled.
	bool bSelfHandle;
	// Holds if this message should be auto accepted.
	bool bAutoAccept;
	// Holds the launch game ID
	FString LaunchGameID;
};
