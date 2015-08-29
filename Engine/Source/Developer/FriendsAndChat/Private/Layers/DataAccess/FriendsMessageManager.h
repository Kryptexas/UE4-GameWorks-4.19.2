// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IChatCommunicationService.h"
#include "FriendsMessageType.h"

// Struct for holding chat message information. Will probably be replaced by OSS version
struct FFriendChatMessage
{
	EChatMessageType::Type MessageType;
	FText FromName;
	FText ToName;
	FText Message;
	FDateTime MessageTime;
	FDateTime ExpireTime;
	TSharedPtr<class FChatMessage> MessageRef;
	TSharedPtr<const FUniqueNetId> SenderId;
	TSharedPtr<const FUniqueNetId> RecipientId;
	bool bIsFromSelf;
};

/**
 * Implement the Friend and Chat manager
 */
class FFriendsMessageManager
	: public TSharedFromThis<FFriendsMessageManager>
	, public IChatCommunicationService
{
public:

	/** Destructor. */
	virtual ~FFriendsMessageManager( ) {};

	virtual void LogIn(IOnlineSubsystem* InOnlineSub, int32 LocalUserNum = 0) = 0;
	virtual void LogOut() = 0;
	virtual const TArray<TSharedRef<FFriendChatMessage> >& GetMessages() const = 0;
	virtual void JoinPublicRoom(const FString& RoomName) = 0;

	DECLARE_EVENT_OneParam(FFriendsMessageManager, FOnChatPublicRoomJoinedEvent, const FString& /*RoomName*/)
	virtual FOnChatPublicRoomJoinedEvent& OnChatPublicRoomJoined() = 0;

	DECLARE_EVENT_OneParam(FFriendsMessageManager, FOnChatPublicRoomExitedEvent, const FString& /*RoomName*/)
	virtual FOnChatPublicRoomExitedEvent& OnChatPublicRoomExited() = 0;
};

/**
 * Creates the implementation of a chat manager.
 *
 * @return the newly created FriendViewModel implementation.
 */
FACTORY(TSharedRef< FFriendsMessageManager >, FFriendsMessageManager, const TSharedRef<class FFriendsAndChatManager>& FriendsAndChatManager);
