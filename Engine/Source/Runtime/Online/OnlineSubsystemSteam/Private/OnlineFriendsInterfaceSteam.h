// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineFriendsInterface.h"
#include "OnlinePresenceInterface.h"
#include "OnlineAsyncTaskManagerSteam.h"
#include "OnlineSubsystemSteamPackage.h"

/**
 * Info associated with an online friend on the Steam service
 */
class FOnlineFriendSteam : 
	public FOnlineFriend
{
public:

	// FOnlineUser

	virtual TSharedRef<FUniqueNetId> GetUserId() const OVERRIDE;
	virtual FString GetRealName() const OVERRIDE;
	virtual FString GetDisplayName() const OVERRIDE;
	virtual bool GetUserAttribute(const FString& AttrName, FString& OutAttrValue) const OVERRIDE;

	// FOnlineFriend
	
	virtual EInviteStatus::Type GetInviteStatus() const OVERRIDE;
	virtual const FOnlineUserPresence& GetPresence() const OVERRIDE;

	// FOnlineFriendMcp

	/**
	 * Init/default constructor
	 */
	FOnlineFriendSteam(const CSteamID& InUserId=CSteamID());

	/**
	 * Get account data attribute
	 *
	 * @param Key account data entry key
	 * @param OutVal [out] value that was found
	 *
	 * @return true if entry was found
	 */
	inline bool GetAccountData(const FString& Key, FString& OutVal) const
	{
		const FString* FoundVal = AccountData.Find(Key);
		if (FoundVal != NULL)
		{
			OutVal = *FoundVal;
			return true;
		}
		return false;
	}

	/** User Id represented as a FUniqueNetId */
	TSharedRef<FUniqueNetId> UserId;
	/** Any addition account data associated with the friend */
	TMap<FString, FString> AccountData;
	/** @temp presence info */
	FOnlineUserPresence Presence;
};

/**
 * Implements the Steam specific interface for friends
 */
class FOnlineFriendsSteam :
	public IOnlineFriends
{
	/** Reference to the main Steam subsystem */
	class FOnlineSubsystemSteam* SteamSubsystem;
	/** The steam user interface to use when interacting with steam */
	class ISteamUser* SteamUserPtr;
	/** The steam friends interface to use when interacting with steam */
	class ISteamFriends* SteamFriendsPtr;
	/** list of friends */
	struct FSteamFriendsList
	{
		TArray< TSharedRef<FOnlineFriendSteam> > Friends;
	};
	/** map of local user idx to friends */
	TMap<int32, FSteamFriendsList> FriendsLists;

	friend class FOnlineAsyncTaskSteamReadFriendsList;

PACKAGE_SCOPE:

	FOnlineFriendsSteam();

public:

	/**
	 * Initializes the various interfaces
	 *
	 * @param InSteamSubsystem the subsystem that owns this object
	 */
	FOnlineFriendsSteam(FOnlineSubsystemSteam* InSteamSubsystem);

	virtual ~FOnlineFriendsSteam() {};

	// IOnlineFriends

	virtual bool ReadFriendsList(int32 LocalUserNum, const FString& ListName) OVERRIDE;
	virtual bool DeleteFriendsList(int32 LocalUserNum, const FString& ListName) OVERRIDE;
	virtual bool SendInvite(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName) OVERRIDE;
	virtual bool AcceptInvite(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName) OVERRIDE;
 	virtual bool RejectInvite(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName) OVERRIDE;
 	virtual bool DeleteFriend(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName) OVERRIDE;
	virtual bool GetFriendsList(int32 LocalUserNum, const FString& ListName, TArray< TSharedRef<FOnlineFriend> >& OutFriends) OVERRIDE;
	virtual TSharedPtr<FOnlineFriend> GetFriend(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName) OVERRIDE;
	virtual bool IsFriend(int32 LocalUserNum, const FUniqueNetId& FriendId, const FString& ListName) OVERRIDE;
};

typedef TSharedPtr<FOnlineFriendsSteam, ESPMode::ThreadSafe> FOnlineFriendsSteamPtr;

/**
 * Fires the delegates for the task on the game thread
 */
class FOnlineAsyncTaskSteamReadFriendsList :
	public FOnlineAsyncTask
{
	/** Interface pointer to trigger the delegates on */
	FOnlineFriendsSteam* FriendsPtr;
	/** The user that is triggering the event */
	int32 LocalUserNum;

public:
	/**
	 * Inits the pointer used to trigger the delegates on
	 *
	 * @param InFriendsPtr the interface to call the delegates on
	 * @param InLocalUserNum the local user that requested the read
	 */
	FOnlineAsyncTaskSteamReadFriendsList(FOnlineFriendsSteam* InFriendsPtr, int32 InLocalUserNum) :
		FriendsPtr(InFriendsPtr),
		LocalUserNum(InLocalUserNum)
	{
		check(FriendsPtr);
	}

	// FOnlineAsyncTask

	virtual FString ToString(void) const OVERRIDE
	{
		return TEXT("FOnlineFriendsSteam::ReadFriendsList() async task completed successfully");
	}

	virtual bool IsDone(void) OVERRIDE
	{
		return true;
	}

	virtual bool WasSuccessful(void) OVERRIDE
	{
		return true;
	}

	virtual void Finalize() OVERRIDE;
	virtual void TriggerDelegates(void) OVERRIDE;
};
