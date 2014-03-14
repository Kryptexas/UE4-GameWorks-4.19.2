// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemUtilsPrivatePCH.h"
#include "ModuleManager.h"
#include "Online.h"
#include "TestFriendsInterface.h"
#include "OnlinePresenceInterface.h"

void FTestFriendsInterface::Test(const TArray<FString>& Invites)
{
	OnlineSub = IOnlineSubsystem::Get(SubsystemName.Len() ? FName(*SubsystemName, FNAME_Find) : NAME_None);
	if (OnlineSub != NULL &&
		OnlineSub->GetIdentityInterface().IsValid() &&
		OnlineSub->GetFriendsInterface().IsValid())
	{
		// don't affect default friends list for MCP
		if (SubsystemName.Equals(TEXT("MCP"),ESearchCase::IgnoreCase))
		{
			FriendsListName = TEXT("TestFriends");
		}

		// Add our delegate for the async call
		OnReadFriendsCompleteDelegate = FOnReadFriendsListCompleteDelegate::CreateRaw(this, &FTestFriendsInterface::OnReadFriendsComplete);
		OnAcceptInviteCompleteDelegate = FOnAcceptInviteCompleteDelegate::CreateRaw(this, &FTestFriendsInterface::OnAcceptInviteComplete);
		OnSendInviteCompleteDelegate = FOnSendInviteCompleteDelegate::CreateRaw(this, &FTestFriendsInterface::OnSendInviteComplete);
		OnDeleteFriendCompleteDelegate = FOnDeleteFriendCompleteDelegate::CreateRaw(this, &FTestFriendsInterface::OnDeleteFriendComplete);
		OnDeleteFriendsListCompleteDelegate = FOnDeleteFriendsListCompleteDelegate::CreateRaw(this, &FTestFriendsInterface::OnDeleteFriendsListComplete);

		OnlineSub->GetFriendsInterface()->AddOnReadFriendsListCompleteDelegate(0, OnReadFriendsCompleteDelegate);
		OnlineSub->GetFriendsInterface()->AddOnAcceptInviteCompleteDelegate(0, OnAcceptInviteCompleteDelegate);
		OnlineSub->GetFriendsInterface()->AddOnSendInviteCompleteDelegate(0, OnSendInviteCompleteDelegate);
		OnlineSub->GetFriendsInterface()->AddOnDeleteFriendCompleteDelegate(0, OnDeleteFriendCompleteDelegate);
		OnlineSub->GetFriendsInterface()->AddOnDeleteFriendsListCompleteDelegate(0, OnDeleteFriendsListCompleteDelegate);

		// list of pending users to send invites to
		for (int32 Idx=0; Idx < Invites.Num(); Idx++)
		{
			TSharedPtr<FUniqueNetId> FriendId = OnlineSub->GetIdentityInterface()->CreateUniquePlayerId(Invites[Idx]);
			if (FriendId.IsValid())
			{
				InvitesToSend.Add(FriendId);
			}
		}

		// kick off next test
		StartNextTest();
	}
	else
	{
		UE_LOG(LogOnline, Warning,
			TEXT("Failed to get friends interface for %s"), *SubsystemName);
		
		FinishTest();
	}
}

void FTestFriendsInterface::StartNextTest()
{
	if (bReadFriendsList)
	{
		OnlineSub->GetFriendsInterface()->ReadFriendsList(0, FriendsListName);
	}
	else if (bAcceptInvites && InvitesToAccept.Num() > 0)
	{
		OnlineSub->GetFriendsInterface()->AcceptInvite(0, *InvitesToAccept[0], FriendsListName);
	}
	else if (bSendInvites && InvitesToSend.Num() > 0)
	{
		OnlineSub->GetFriendsInterface()->SendInvite(0, *InvitesToSend[0], FriendsListName);
	}
	else if (bDeleteFriends && FriendsToDelete.Num() > 0)
	{
		OnlineSub->GetFriendsInterface()->DeleteFriend(0, *FriendsToDelete[0], FriendsListName);
	}
	else if (bDeleteFriendsList)
	{
		OnlineSub->GetFriendsInterface()->DeleteFriendsList(0, FriendsListName);
	}
	else
	{
		FinishTest();
	}

}

void FTestFriendsInterface::FinishTest()
{
	if (OnlineSub != NULL &&
		OnlineSub->GetFriendsInterface().IsValid())
	{
		// Clear delegates for the various async calls
		OnlineSub->GetFriendsInterface()->ClearOnReadFriendsListCompleteDelegate(0, OnReadFriendsCompleteDelegate);
		OnlineSub->GetFriendsInterface()->ClearOnAcceptInviteCompleteDelegate(0, OnAcceptInviteCompleteDelegate);
		OnlineSub->GetFriendsInterface()->ClearOnSendInviteCompleteDelegate(0, OnSendInviteCompleteDelegate);
		OnlineSub->GetFriendsInterface()->ClearOnDeleteFriendCompleteDelegate(0, OnDeleteFriendCompleteDelegate);
		OnlineSub->GetFriendsInterface()->ClearOnDeleteFriendsListCompleteDelegate(0, OnDeleteFriendsListCompleteDelegate);
	}
	delete this;
}

void FTestFriendsInterface::OnReadFriendsComplete(int32 LocalPlayer, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr)
{
	UE_LOG(LogOnline, Log,
		TEXT("ReadFriendsList() for player (%d) was success=%d"), LocalPlayer, bWasSuccessful);

	if (bWasSuccessful)
	{
		TArray< TSharedRef<FOnlineFriend> > Friends;
		// Grab the friends data so we can print it out
		if (OnlineSub->GetFriendsInterface()->GetFriendsList(LocalPlayer, ListName, Friends))
		{
			UE_LOG(LogOnline, Log,
				TEXT("GetFriendsList(%d) returned %d friends"), LocalPlayer, Friends.Num());

			// Clear old entries
			InvitesToAccept.Empty();
			FriendsToDelete.Empty();
			// Log each friend's data out
			for (int32 Index = 0; Index < Friends.Num(); Index++)
			{
				const FOnlineFriend& Friend = *Friends[Index];
				const FOnlineUserPresence& Presence = Friend.GetPresence();
				UE_LOG(LogOnline, Log,
					TEXT("\t%s has unique id (%s)"), *Friend.GetDisplayName(), *Friend.GetUserId()->ToDebugString());
				UE_LOG(LogOnline, Log,
					TEXT("\t\t Invite status (%s)"), EInviteStatus::ToString(Friend.GetInviteStatus()));
				UE_LOG(LogOnline, Log,
					TEXT("\t\t Presence: %s"), *Presence.PresenceStr);
				UE_LOG(LogOnline, Log,
					TEXT("\t\t bIsOnline (%s)"), Presence.bIsOnline ? TEXT("true") : TEXT("false"));
				UE_LOG(LogOnline, Log,
					TEXT("\t\t bIsPlaying (%s)"), Presence.bIsPlaying ? TEXT("true") : TEXT("false"));
				UE_LOG(LogOnline, Log,
					TEXT("\t\t bIsPlayingThisGame (%s)"), Presence.bIsPlayingThisGame ? TEXT("true") : TEXT("false"));
				UE_LOG(LogOnline, Log,
					TEXT("\t\t bIsJoinable (%s)"), Presence.bIsJoinable ? TEXT("true") : TEXT("false"));
				UE_LOG(LogOnline, Log,
					TEXT("\t\t bHasVoiceSupport (%s)"), Presence.bHasVoiceSupport ? TEXT("true") : TEXT("false"));

				// keep track of pending invites from the list and accept them
				if (Friend.GetInviteStatus() == EInviteStatus::PendingInbound)
				{
					InvitesToAccept.AddUnique(Friend.GetUserId());
				}
				// keep track of list of friends to delete
				FriendsToDelete.AddUnique(Friend.GetUserId());
			}
		}	
		else
		{
			UE_LOG(LogOnline, Log,
				TEXT("GetFriendsList(%d) failed"), LocalPlayer);
		}
	}
	
	// done with this part of the test
	bReadFriendsList = false;
	// kick off next test
	StartNextTest();
}

void FTestFriendsInterface::OnAcceptInviteComplete(int32 LocalPlayer, bool bWasSuccessful, const FUniqueNetId& FriendId, const FString& ListName, const FString& ErrorStr)
{
	UE_LOG(LogOnline, Log, TEXT("AcceptInvite() for player (%d) from friend (%s) was success=%d. %s"), 
		LocalPlayer, *FriendId.ToDebugString(), bWasSuccessful, *ErrorStr);

	// done with this part of the test if no more invites to accept
	InvitesToAccept.RemoveAt(0);
	if (InvitesToAccept.Num() == 0)
	{
		bAcceptInvites = false;
		bReadFriendsList = true;
	}
	// kick off next test
	StartNextTest();
}

void FTestFriendsInterface::OnSendInviteComplete(int32 LocalPlayer, bool bWasSuccessful, const FUniqueNetId& FriendId, const FString& ListName, const FString& ErrorStr)
{
	UE_LOG(LogOnline, Log, TEXT("SendInvite() for player (%d) to friend (%s) was success=%d. %s"), 
		LocalPlayer, *FriendId.ToDebugString(), bWasSuccessful, *ErrorStr);

	// done with this part of the test if no more invites to send
	InvitesToSend.RemoveAt(0);
	if (InvitesToSend.Num() == 0)
	{
		bSendInvites = false;
		bReadFriendsList = true;
	}
	// kick off next test
	StartNextTest();
}

void FTestFriendsInterface::OnDeleteFriendComplete(int32 LocalPlayer, bool bWasSuccessful, const FUniqueNetId& FriendId, const FString& ListName, const FString& ErrorStr)
{
	UE_LOG(LogOnline, Log, TEXT("DeleteFriend() for player (%d) to friend (%s) was success=%d. %s"), 
		LocalPlayer, *FriendId.ToDebugString(), bWasSuccessful, *ErrorStr);

	// done with this part of the test if no more invites to send
	FriendsToDelete.RemoveAt(0);
	if (FriendsToDelete.Num() == 0)
	{
		bDeleteFriends = false;
		bReadFriendsList = true;
	}
	// kick off next test
	StartNextTest();
}

void FTestFriendsInterface::OnDeleteFriendsListComplete(int32 LocalPlayer, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr)
{
	UE_LOG(LogOnline, Log,
		TEXT("DeleteFriendsList() for player (%d) was success=%d"), LocalPlayer, bWasSuccessful);

	// done with this part of the test
	bDeleteFriendsList = false;
	// kick off next test
	StartNextTest();
}

