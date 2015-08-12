// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "FriendPartyInviteItem.h"

#include "GameAndPartyService.h"

// FFriendPartyInviteItem

bool FFriendPartyInviteItem::CanJoinParty() const
{
	TSharedPtr<IOnlinePartyJoinInfo> PartyInfo = GetPartyJoinInfo();
	return PartyInfo.IsValid() && !PartyInfo->IsInvalidForJoin() && !GameAndPartyService->IsFriendInSameParty(AsShared());
}

bool FFriendPartyInviteItem::CanInvite() const
{
	FString FriendsClientID = GetClientId();
	return FriendsClientID == GameAndPartyService->GetUserClientId() || FFriendItem::LauncherClientIds.Contains(FriendsClientID);
}

bool FFriendPartyInviteItem::IsGameRequest() const
{
	return true;
}

bool FFriendPartyInviteItem::IsGameJoinable() const
{
	//@todo samz1 - make sure party invite hasn't already been accepted/rejected

	if (PartyJoinInfo.IsValid())
	{
		const TSharedPtr<const IFriendItem> Item = AsShared();
		return !GameAndPartyService->IsFriendInSameParty(Item);
	}
	return false;
}

TSharedPtr<const FUniqueNetId> FFriendPartyInviteItem::GetGameSessionId() const
{
	return nullptr;
}

TSharedPtr<IOnlinePartyJoinInfo> FFriendPartyInviteItem::GetPartyJoinInfo() const
{
	return PartyJoinInfo;
}

const FString FFriendPartyInviteItem::GetClientId() const
{
	return ClientId;
}