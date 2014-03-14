// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once


#include "OnlineFriendsInterface.h"
#include "OnlineSubsystemIOSTypes.h"
#include "OnlinePresenceInterface.h"

/**
 * Info associated with an online friend on the ios gamecenter service
 */
class FOnlineFriendIOS : 
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
	FOnlineFriendIOS(const FString& InUserId=TEXT("")) 
		: UserId(new FUniqueNetIdString(InUserId))
	{
	}

	/**
	 * Destructor
	 */
	virtual ~FOnlineFriendIOS()
	{
	}

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

class FOnlineFriendsIOS :
	public IOnlineFriends
{
private:

	/** Reference to the main IOS identity */
	class FOnlineIdentityIOS* IdentityInterface;

	/** The collection of game center friends received through the GK callbacks in ReadFriendsList */
	TArray< TSharedRef<FOnlineFriendIOS> > CachedFriends;


public:

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

	// FOnlineFriendsIOS

	/**
	 * Destructor
	 */
	virtual ~FOnlineFriendsIOS() {};

PACKAGE_SCOPE:

	/**
	 * Constructor
	 *
	 * @param InSubsystem - A pointer to the subsystem which has the features we need to gather friends. (Identity interface etc.)
	 */
	FOnlineFriendsIOS(FOnlineSubsystemIOS* InSubsystem);
};

typedef TSharedPtr<FOnlineFriendsIOS, ESPMode::ThreadSafe> FOnlineFriendsIOSPtr;