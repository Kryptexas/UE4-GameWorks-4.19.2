// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IFriendItem.h"
/**
 * Class containing a friend's party invite information - used to build the list view.
 */
class FFriendPartyInviteItem : 
	public FFriendItem
{
public:

	// FFriendStuct overrides

	virtual bool IsGameRequest() const override;
	virtual bool IsGameJoinable() const override;
	virtual TSharedPtr<const FUniqueNetId> GetGameSessionId() const override;
	virtual TSharedPtr<IOnlinePartyJoinInfo> GetPartyJoinInfo() const override;
	virtual const FString GetClientId() const override;

	// FFriendPartyInviteItem

	FFriendPartyInviteItem(
		const TSharedRef<FOnlineUser>& InOnlineUser,
		const FString& InClientId,
		const TSharedRef<IOnlinePartyJoinInfo>& InPartyJoinInfo,
		const TSharedRef<class FFriendsAndChatManager>& FriendsAndChatManager)
		: FFriendItem(nullptr, InOnlineUser, EFriendsDisplayLists::GameInviteDisplay, FriendsAndChatManager)
		, PartyJoinInfo(InPartyJoinInfo)
		, ClientId(InClientId)
	{ }

protected:

	/** Hidden default constructor. */
	FFriendPartyInviteItem()
	{ }

private:

	TSharedPtr<IOnlinePartyJoinInfo> PartyJoinInfo;
	FString ClientId;
};
