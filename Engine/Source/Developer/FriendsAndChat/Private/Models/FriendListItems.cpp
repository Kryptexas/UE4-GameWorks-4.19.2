// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"


#define LOCTEXT_NAMESPACE "FriendListItems"


// FFriendStruct implementation

void FFriendStuct::AddChild( TSharedPtr< FFriendStuct > InChild )
{
	Children.Add( InChild );
}

const TSharedPtr< FOnlineFriend > FFriendStuct::GetOnlineFriend() const
{
	return OnlineFriend;
}

const TSharedPtr< FOnlineUser > FFriendStuct::GetOnlineUser() const
{
	return OnlineUser;
}

TArray< TSharedPtr < FFriendStuct > >& FFriendStuct::GetChildList()
{
	return Children;
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
		if(OnlinePresence.bIsPlaying)
		{
			return FText::FromString("Is Playing");
		}
		else
		{
			return FText::FromString("Not Playing");
		}
	}
	return FText::GetEmpty();
}

const bool FFriendStuct::IsOnline() const
{
	if(OnlineFriend.IsValid())
	{
		return OnlineFriend->GetPresence().bIsOnline;
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
