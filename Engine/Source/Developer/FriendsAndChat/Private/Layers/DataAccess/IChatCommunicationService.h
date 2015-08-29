// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Interface for the message communication service
 */
class IChatCommunicationService
{
public:

	virtual bool SendRoomMessage(const FString& RoomName, const FString& MsgBody) = 0;
	virtual bool SendPrivateMessage(TSharedPtr<const FUniqueNetId> UserID, const FText MessageText) = 0;
	virtual void InsertNetworkMessage(const FString& MsgBody) = 0;

	DECLARE_EVENT_OneParam(IChatCommunicationService, FOnChatMessageReceivedEvent, const TSharedRef<struct FFriendChatMessage> /*The chat message*/)
	virtual FOnChatMessageReceivedEvent& OnChatMessageRecieved() = 0;
};
