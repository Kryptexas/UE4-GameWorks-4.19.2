// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Enum for the chat message type.
 */
namespace EChatMessageType
{
	enum Type : uint8
	{
		// Person whisper Item
		Whisper,
		// Party Chat Item
		Party,
		// Global Chat Item
		Global,
	};

	/** @return the FTextified version of the enum passed in */
	inline FText ToText(EChatMessageType::Type Type)
	{
		switch (Type)
		{
			case Global: return NSLOCTEXT("FriendsList","Global", "Global");
			case Whisper: return NSLOCTEXT("FriendsList","Whisper", "Whisper");
			case Party: return NSLOCTEXT("FriendsList","Party", "Party");

			default: return FText::GetEmpty();
		}
	}
};

// Stuct for holding chat message information. Will probably be replaced by OSS version
struct FFriendChatMessage
{
	EChatMessageType::Type MessageType;
	FText FriendID;
	FText Message;
	FText MessageTimeText;
	bool bIsFromSelf;
};

/**
 * Implement the Friend and Chat manager
 */
class FFriendsMessageManager
	: public TSharedFromThis<FFriendsMessageManager>
{
public:

	/** Destructor. */
	virtual ~FFriendsMessageManager( ) {};

	virtual void LogIn() = 0;
	virtual void LogOut() = 0;
	virtual void SendMessage(const FUniqueNetId& RecipientId, const FString& MsgBody) = 0;

	DECLARE_EVENT_OneParam(FFriendsMessageManager, FOnChatMessageReceivedEvent, const TSharedRef<FFriendChatMessage> /*The chat message*/)
	virtual FOnChatMessageReceivedEvent& OnChatMessageRecieved() = 0;

};

/**
 * Creates the implementation of a chat manager.
 *
 * @return the newly created FriendViewModel implementation.
 */
FACTORY(TSharedRef< FFriendsMessageManager >, FFriendsMessageManager);
