// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"


#define LOCTEXT_NAMESPACE "FriendListItems"


// FFriendStruct implementation

const FString FFriendItem::LauncherClientIds("f3e80378aed4462498774a7951cd263f, 34a02cf8f4414e29b15921876da36f9a");
const FString FFriendItem::FortniteLiveClientId("ec684b8c687f479fadea3cb2ad83f5c6");
const FString FFriendItem::FortniteDeprecatedLiveClientId("300d79839c914445948e3c1100f211db");
const FString FFriendItem::FortnitePublicTestClientId("7cf6977d137640759a51ec8fc88ebf9e");
const FString FFriendItem::FortniteClientIds("300d79839c914445948e3c1100f211db, eb1ce404a0e5446f8e43964a24db7bdd, 299a8d0ee5c84d43a94ffe9e54cc92dc, 3aa8093ea5f14165a3b5f77ba7f6c66a, 81545b947bda44a6bacd844af10e0716, df94a22141b24b02acfffc811d4d6159, 5a1fb298e1824563ad28296cb3a4008e, 807553cb40704839adcfc505c29920be, 7cf6977d137640759a51ec8fc88ebf9e, ec684b8c687f479fadea3cb2ad83f5c6, e02dd6fea2bb4f029bf529992cff8351, 807553cb40704839adcfc505c29920be, 7cf6977d137640759a51ec8fc88ebf9e, ec684b8c687f479fadea3cb2ad83f5c6, e02dd6fea2bb4f029bf529992cff8351");
const FString FFriendItem::UnrealTournamentClientId("1252412dc7704a9690f6ea4611bc81ee");

const TSharedPtr< FOnlineFriend > FFriendItem::GetOnlineFriend() const
{
	return OnlineFriend;
}

const TSharedPtr< FOnlineUser > FFriendItem::GetOnlineUser() const
{
	return OnlineUser;
}

const FString FFriendItem::GetName() const
{
	if ( OnlineUser.IsValid() )
	{
		return OnlineUser->GetDisplayName();
	}
	
	return GroupName;
}

const FText FFriendItem::GetFriendLocation() const
{
	if(OnlineFriend.IsValid())
	{
		const FOnlineUserPresence& OnlinePresence = OnlineFriend->GetPresence();

		// TODO - Localize text once final format is decided
		if (OnlinePresence.bIsOnline)
		{
			switch (OnlinePresence.Status.State)
			{
			case EOnlinePresenceState::Offline:
				return FText::FromString("Offline");
			case EOnlinePresenceState::Away:
			case EOnlinePresenceState::ExtendedAway:
				return FText::FromString("Away");
			case EOnlinePresenceState::Chat:
			case EOnlinePresenceState::DoNotDisturb:
			case EOnlinePresenceState::Online:	
			default:
				// rich presence string
				if (!OnlinePresence.Status.StatusStr.IsEmpty())
				{
					return FText::FromString(*OnlinePresence.Status.StatusStr);
				}
				else
				{
					return FText::FromString(TEXT("Online"));
				}
			};
		}
		else
		{
			return FText::FromString("Offline");
		}
	}
	return FText::GetEmpty();
}

const FString FFriendItem::GetClientId() const
{
	FString Result;
	if (OnlineFriend.IsValid())
	{
		const FOnlineUserPresence& OnlinePresence = OnlineFriend->GetPresence();
		const FVariantData* ClientId = OnlinePresence.Status.Properties.Find(DefaultClientIdKey);
		if (ClientId != nullptr &&
			ClientId->GetType() == EOnlineKeyValuePairDataType::String)
		{
			ClientId->GetValue(Result);
		}
	}
	return Result;
}

const FString FFriendItem::GetClientName() const
{
	FString Result;
	FString ClientId = GetClientId();

	if (!ClientId.IsEmpty())
	{
		// hardcoded for now, need a generic way to receive client names or map client ids to names
		if (ClientId == FFriendItem::FortniteLiveClientId || ClientId == FFriendItem::FortniteDeprecatedLiveClientId)
		{
			Result = TEXT("Fortnite");
		}
		else if (ClientId == FFriendItem::FortnitePublicTestClientId)
		{
			Result = TEXT("Fortnite PublicTest");
		}
		else if (FFriendItem::FortniteClientIds.Contains(ClientId))
		{
			Result = TEXT("Fortnite Dev");
		}
		else if (ClientId == FFriendItem::UnrealTournamentClientId)
		{
			Result = TEXT("Unreal Tournament");
		}
		else if (FFriendItem::LauncherClientIds.Contains(ClientId))
		{
			Result = TEXT("Epic Games Launcher");
		}
	}
	return Result;
}

FDateTime FFriendItem::GetLastSeen() const
{
	return FDateTime::UtcNow();
}

const bool FFriendItem::IsOnline() const
{
	if(OnlineFriend.IsValid())
	{
		return OnlineFriend->GetPresence().Status.State != EOnlinePresenceState::Offline ? true : false;
	}
	return false;
}

const EOnlinePresenceState::Type FFriendItem::GetOnlineStatus() const
{
	if (OnlineFriend.IsValid())
	{
		return OnlineFriend->GetPresence().Status.State;
	}
	return EOnlinePresenceState::Offline;
}

bool FFriendItem::IsGameJoinable() const
{
	return false;
}

bool FFriendItem::IsInParty() const
{
	TSharedPtr<IOnlinePartyJoinInfo> PartyInfo = GetPartyJoinInfo();
	return PartyInfo.IsValid();
}

bool FFriendItem::CanJoinParty() const
{
	return false;
}

bool FFriendItem::CanInvite() const
{
	return false;
}

TSharedPtr<const FUniqueNetId> FFriendItem::GetGameSessionId() const
{
	if (OnlineFriend.IsValid())
	{
		const FOnlineUserPresence& FriendPresence = OnlineFriend->GetPresence();
		if (FriendPresence.SessionId.IsValid())
		{
			return FriendPresence.SessionId;
		}
	}
	return nullptr;
}

TSharedPtr<IOnlinePartyJoinInfo> FFriendItem::GetPartyJoinInfo() const
{
	return nullptr;
}

const TSharedRef<const FUniqueNetId> FFriendItem::GetUniqueID() const
{
	return UniqueID.ToSharedRef();
}

const EFriendsDisplayLists::Type FFriendItem::GetListType() const
{
	return ListType;
}

void FFriendItem::SetOnlineFriend( TSharedPtr< FOnlineFriend > InOnlineFriend )
{
	OnlineFriend = InOnlineFriend;
	bIsUpdated = true;
}

void FFriendItem::SetOnlineUser(TSharedPtr< FOnlineUser > InOnlineUser)
{
	OnlineUser = InOnlineUser;
}

void FFriendItem::ClearUpdated()
{
	bIsUpdated = false;
	bIsPendingAccepted = false;
	bIsPendingInvite = false;
	bIsPendingDelete = false;
}

bool FFriendItem::IsUpdated()
{
	return bIsUpdated;
}

void FFriendItem::SetPendingAccept()
{
	bIsPendingAccepted = true;
}

bool FFriendItem::IsPendingAccepted() const
{
	return bIsPendingAccepted;
}

bool FFriendItem::IsGameRequest() const
{
	return false;
}

void FFriendItem::SetPendingInvite()
{
	bIsPendingInvite = true;
}

void FFriendItem::SetPendingDelete()
{
	bIsPendingDelete = true;
}

bool FFriendItem::IsPendingDelete() const
{
	return bIsPendingDelete;
}

EInviteStatus::Type FFriendItem::GetInviteStatus()
{
	if ( bIsPendingAccepted )
	{
		return EInviteStatus::Accepted;
	}
	else if ( bIsPendingInvite )
	{
		return EInviteStatus::PendingOutbound;
	}
	else if ( OnlineFriend.IsValid() )
	{
		return OnlineFriend->GetInviteStatus();
	}

	return EInviteStatus::Unknown;
}

#undef LOCTEXT_NAMESPACE
