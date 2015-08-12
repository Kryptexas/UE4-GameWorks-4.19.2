// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"
#include "FriendFriendItem.h"

#include "FriendsService.h"

#define LOCTEXT_NAMESPACE ""


// FFriendStruct implementation

bool FFriendFriendItem::IsGameJoinable() const
{
	if (GetOnlineFriend().IsValid())
	{
		const FOnlineUserPresence& FriendPresence = GetOnlineFriend()->GetPresence();
		const bool bIsOnline = FriendPresence.Status.State != EOnlinePresenceState::Offline;
		if (bIsOnline)
		{
			const bool bIsGameJoinable = FriendPresence.bIsJoinable && !FriendsService->IsFriendInSameSession(AsShared());
			if (bIsGameJoinable)
			{
				return true;
			}
		}
	}
	return false;
}

bool FFriendFriendItem::CanJoinParty() const
{
	TSharedPtr<IOnlinePartyJoinInfo> PartyInfo = GetPartyJoinInfo();
	return PartyInfo.IsValid() && !PartyInfo->IsInvalidForJoin() && !FriendsService->IsFriendInSameParty(AsShared());
}

bool FFriendFriendItem::CanInvite() const
{
	FString FriendsClientID = GetClientId();
	return FriendsClientID == FriendsService->GetUserClientId() || FFriendItem::LauncherClientIds.Contains(FriendsClientID);
}

TSharedPtr<IOnlinePartyJoinInfo> FFriendFriendItem::GetPartyJoinInfo() const
{
	// obtain party info from presence
	return FriendsService->GetPartyJoinInfo(AsShared());
}

#undef LOCTEXT_NAMESPACE

