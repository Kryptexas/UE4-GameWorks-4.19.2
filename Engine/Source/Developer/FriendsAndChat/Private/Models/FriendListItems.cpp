// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"


#define LOCTEXT_NAMESPACE "FriendListItems"


// FFriendStruct implementation

const TSharedPtr< FOnlineFriend > FFriendStuct::GetOnlineFriend() const
{
	return OnlineFriend;
}

const TSharedPtr< FOnlineUser > FFriendStuct::GetOnlineUser() const
{
	return OnlineUser;
}

const FString FFriendStuct::GetName() const
{
	if ( OnlineUser.IsValid() )
	{
		return OnlineUser->GetDisplayName();
	}
	
	return GroupName;
}

const FText FFriendStuct::GetFriendLocation() const
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

const FString FFriendStuct::GetClientId() const
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

const bool FFriendStuct::IsOnline() const
{
	if(OnlineFriend.IsValid())
	{
		return OnlineFriend->GetPresence().Status.State == EOnlinePresenceState::Online ? true : false;
	}
	return false;
}

const TSharedRef< FUniqueNetId > FFriendStuct::GetUniqueID() const
{
	return UniqueID.ToSharedRef();
}

const EFriendsDisplayLists::Type FFriendStuct::GetListType() const
{
	return ListType;
}

void FFriendStuct::SetOnlineFriend( TSharedPtr< FOnlineFriend > InOnlineFriend )
{
	OnlineFriend = InOnlineFriend;
	bIsUpdated = true;
}

void FFriendStuct::SetOnlineUser(TSharedPtr< FOnlineUser > InOnlineUser)
{
	OnlineUser = InOnlineUser;
}

void FFriendStuct::ClearUpdated()
{
	bIsUpdated = false;
	bIsPendingAccepted = false;
	bIsPendingInvite = false;
	bIsPendingDelete = false;
}

bool FFriendStuct::IsUpdated()
{
	return bIsUpdated;
}

void FFriendStuct::SetPendingAccept()
{
	bIsPendingAccepted = true;
}

bool FFriendStuct::IsPendingAccepted() const
{
	return bIsPendingAccepted;
}

bool FFriendStuct::IsGameRequest() const
{
	return bIsGameRequest;
}

void FFriendStuct::SetPendingInvite()
{
	bIsPendingInvite = true;
}

void FFriendStuct::SetPendingDelete()
{
	bIsPendingDelete = true;
}

bool FFriendStuct::IsPendingDelete() const
{
	return bIsPendingDelete;
}

EInviteStatus::Type FFriendStuct::GetInviteStatus()
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
