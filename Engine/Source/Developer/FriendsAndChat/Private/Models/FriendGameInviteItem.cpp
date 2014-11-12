// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "FriendGameInviteItem.h"

// FFriendGameInviteItem

bool FFriendGameInviteItem::IsGameRequest() const
{
	return true;
}

bool FFriendGameInviteItem::IsGameJoinable() const
{
	return !GetGameSessionId().IsEmpty();
}

FString FFriendGameInviteItem::GetGameSessionId() const
{
	FString SessionId;
	if (SessionResult->Session.SessionInfo->IsValid())
	{
		SessionId = SessionResult->Session.SessionInfo->GetSessionId().ToString();
	}
	return SessionId;
}
