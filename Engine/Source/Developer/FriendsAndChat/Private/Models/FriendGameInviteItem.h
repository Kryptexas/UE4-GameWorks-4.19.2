// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IFriendItem.h"
/**
 * Class containing a friend's game invite information - used to build the list view.
 */
class FFriendGameInviteItem : 
	public FFriendItem
{
public:

	// FFriendStuct overrides

	virtual bool IsGameRequest() const override;
	virtual bool IsGameJoinable() const override;
	virtual TSharedPtr<const FUniqueNetId> GetGameSessionId() const override;
	virtual const FString GetClientId() const override;
	// FFriendGameInviteItem

	FFriendGameInviteItem(
		const TSharedRef<FOnlineUser>& InOnlineUser,
		const TSharedRef<FOnlineSessionSearchResult>& InSessionResult,
		const FString& InClientId,
		const TSharedRef<FOnlineFriend>& InOnlineFriend,
		const TSharedRef<class FFriendsAndChatManager> FriendsAndChatManager)
		: FFriendItem(InOnlineFriend, InOnlineUser, EFriendsDisplayLists::GameInviteDisplay, FriendsAndChatManager)
		, SessionResult(InSessionResult)
		, ClientId(InClientId)
		, FriendsAndChatManager(FriendsAndChatManager)
	{ }

private:

	TSharedPtr<FOnlineSessionSearchResult> SessionResult;
	TWeakPtr<class FFriendsAndChatManager> FriendsAndChatManager;
	FString ClientId;
};
