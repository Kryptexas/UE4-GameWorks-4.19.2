// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "OnlineSubsystemIOSPrivatePCH.h"

// FOnlineFriendIOS

TSharedRef<FUniqueNetId> FOnlineFriendIOS::GetUserId() const
{
	return UserId;
}

FString FOnlineFriendIOS::GetRealName() const
{
	FString Result;
	GetAccountData(TEXT("nickname"), Result);
	return Result;
}

FString FOnlineFriendIOS::GetDisplayName() const
{
	FString Result;
	GetAccountData(TEXT("nickname"), Result);
	return Result;
}

bool FOnlineFriendIOS::GetUserAttribute(const FString& AttrName, FString& OutAttrValue) const
{
	return GetAccountData(AttrName, OutAttrValue);
}

EInviteStatus::Type FOnlineFriendIOS::GetInviteStatus() const
{
	return EInviteStatus::Accepted;
}

const FOnlineUserPresence& FOnlineFriendIOS::GetPresence() const
{
	return Presence;
}

// FOnlineFriendsIOS

FOnlineFriendsIOS::FOnlineFriendsIOS(FOnlineSubsystemIOS* InSubsystem)
{
	UE_LOG(LogOnline, Verbose, TEXT("FOnlineFriendsIOS::FOnlineFriendsIOS()"));

	IdentityInterface = (FOnlineIdentityIOS*)InSubsystem->GetIdentityInterface().Get();
}

bool FOnlineFriendsIOS::ReadFriendsList(int32 LocalUserNum, const FString& ListName)
{
	UE_LOG(LogOnline, Verbose, TEXT("FOnlineFriendsIOS::ReadFriendsList()"));
	bool bSuccessfullyBeganReadFriends = false;

	if( IdentityInterface->GetLocalGameCenterUser() != NULL && IdentityInterface->GetLocalGameCenterUser().isAuthenticated )
	{
		bSuccessfullyBeganReadFriends = true;
		dispatch_async(dispatch_get_main_queue(), ^
		{
			// Get the friends list for the local player from the server
			[[GKLocalPlayer localPlayer] loadFriendsWithCompletionHandler:
				^(NSArray* Friends, NSError* Error) 
				{
					if( Error )
					{
						FString ErrorStr(FString::Printf(TEXT("FOnlineFriendsIOS::ReadFriendsList() - Failed to read friends list with error: [%i]"), [Error code]));
						UE_LOG(LogOnline, Verbose, TEXT("%s"), *ErrorStr);

						// Report back to the game thread whether this succeeded.
						[FIOSAsyncTask CreateTaskWithBlock : ^ bool(void)
						{
							TriggerOnReadFriendsListCompleteDelegates(0, false, ListName, ErrorStr);
							return true;
						}];
					}
					else
					{
						[GKPlayer loadPlayersForIdentifiers:Friends withCompletionHandler:
							^(NSArray* Players, NSError* Error2)
							{
								FString ErrorStr;
								bool bWasSuccessful = false;
								if( Error2 )
								{
									ErrorStr = FString::Printf(TEXT("FOnlineFriendsIOS::ReadFriendsList() - Failed to loadPlayersForIdentifiers with error: [%i]"), [Error2 code]);
								}
								else
								{
									// Clear our previosly cached friends before we repopulate the cache.
									CachedFriends.Empty();
									for( int32 FriendIdx = 0; FriendIdx < [Players count]; FriendIdx++ )
									{
										GKPlayer* Friend = Players[ FriendIdx ];

										// Add new friend entry to list
										TSharedRef<FOnlineFriendIOS> FriendEntry(
											new FOnlineFriendIOS(ANSI_TO_TCHAR([Friend.playerID cStringUsingEncoding:NSASCIIStringEncoding]))
											);
										FriendEntry->AccountData.Add(
											TEXT("nickname"), ANSI_TO_TCHAR([Friend.alias cStringUsingEncoding:NSASCIIStringEncoding])
											);
										CachedFriends.Add(FriendEntry);

										UE_LOG(LogOnline, Verbose, TEXT("GCFriend - Id:%s Alias:%s"), ANSI_TO_TCHAR([Friend.playerID cStringUsingEncoding:NSASCIIStringEncoding]), 
											*FriendEntry->GetDisplayName() );
									}

									bWasSuccessful = true;
								}
								
								// Report back to the game thread whether this succeeded.
								[FIOSAsyncTask CreateTaskWithBlock : ^ bool(void)
								{
									TriggerOnReadFriendsListCompleteDelegates(0, bWasSuccessful, ListName, ErrorStr);
									return true;
								}];
							}
						];
					}
				}
			];
		});
	}
	else
	{
		// no gamecenter login means we cannot read the friends.
		TriggerOnReadFriendsListCompleteDelegates( 0 , false, ListName, TEXT("not logged in") );
	}

	return bSuccessfullyBeganReadFriends;
}

bool FOnlineFriendsIOS::DeleteFriendsList(int32 LocalUserNum, const FString& ListName)
{
	TriggerOnDeleteFriendsListCompleteDelegates(LocalUserNum, false, ListName, FString(TEXT("DeleteFriendsList() is not supported")));
	return false;
}

bool FOnlineFriendsIOS::SendInvite(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName)
{
	TriggerOnSendInviteCompleteDelegates(LocalUserNum, false, FriendId, ListName, FString(TEXT("SendInvite() is not supported")));
	return false;
}

bool FOnlineFriendsIOS::AcceptInvite(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName)
{
	TriggerOnAcceptInviteCompleteDelegates(LocalUserNum, false, FriendId, ListName, FString(TEXT("AcceptInvite() is not supported")));
	return false;
}

bool FOnlineFriendsIOS::RejectInvite(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName)
{
	TriggerOnRejectInviteCompleteDelegates(LocalUserNum, false, FriendId, ListName, FString(TEXT("RejectInvite() is not supported")));
	return false;
}

bool FOnlineFriendsIOS::DeleteFriend(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName)
{
	TriggerOnDeleteFriendCompleteDelegates(LocalUserNum, false, FriendId, ListName, FString(TEXT("DeleteFriend() is not supported")));
	return false;
}

bool FOnlineFriendsIOS::GetFriendsList(int32 LocalUserNum, const FString& ListName, TArray< TSharedRef<FOnlineFriend> >& OutFriends)
{
	UE_LOG(LogOnline, Verbose, TEXT("FOnlineFriendsIOS::GetFriendsList()"));

	for (int32 Idx=0; Idx < CachedFriends.Num(); Idx++)
	{
		OutFriends.Add(CachedFriends[Idx]);
	}

	return true;
}

TSharedPtr<FOnlineFriend> FOnlineFriendsIOS::GetFriend(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName)
{
	TSharedPtr<FOnlineFriend> Result;

	UE_LOG(LogOnline, Verbose, TEXT("FOnlineFriendsIOS::GetFriend()"));

	for (int32 Idx=0; Idx < CachedFriends.Num(); Idx++)
	{
		if (*(CachedFriends[Idx]->GetUserId()) == FriendId)
		{
			Result = CachedFriends[Idx];
			break;
		}
	}

	return Result;
}

bool FOnlineFriendsIOS::IsFriend(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName)
{
	UE_LOG(LogOnline, Verbose, TEXT("FOnlineFriendsIOS::IsFriend()"));

	TSharedPtr<FOnlineFriend> Friend = GetFriend(LocalUserNum,FriendId,ListName);
	if (Friend.IsValid() &&
		Friend->GetInviteStatus() == EInviteStatus::Accepted)
	{
		return true;
	}
	return false;
}

