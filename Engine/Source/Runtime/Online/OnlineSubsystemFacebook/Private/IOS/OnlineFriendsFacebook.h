// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once


#include "OnlineFriendsInterface.h"
#include "OnlineSharingFacebook.h"
#include "OnlinePresenceInterface.h"

/**
 * Info associated with an online friend on the facebook service
 */
class FOnlineFriendFacebook : 
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
	FOnlineFriendFacebook(const FString& InUserId=TEXT("")) 
		: UserId(new FUniqueNetIdString(InUserId))
	{
	}

	/**
	 * Destructor
	 */
	virtual ~FOnlineFriendFacebook()
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


/**
 * Facebook service implementation of the online friends interface
 */
class FOnlineFriendsFacebook :
	public IOnlineFriends
{

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

	// FOnlineFriendsFacebook

PACKAGE_SCOPE:

	/**
	 * Constructor
	 *
	 * @param InSubsystem Facebook subsystem being used
	 */
	FOnlineFriendsFacebook(class FOnlineSubsystemFacebook* InSubsystem);


public:
	
	/**
	 * Destructor
	 */
	virtual ~FOnlineFriendsFacebook();


private:

	/**
	 * Should use the OSS param constructor instead
	 */
	FOnlineFriendsFacebook();


private:

	/**
	 * Delegate fired when we have a response after we have requested read permissions of the users friends list
	 *
	 * @param LocalUserNum		- The local player number the friends were read for
	 * @param bWasSuccessful	- Whether the permissions were updated successfully
	 */
	void OnReadFriendsPermissionsUpdated(int32 LocalUserNum, bool bWasSuccessful);

	/**
	 * Using the open graph for fb to read friends list, called when we have ensured correct permissions
	 *
	 * @param LocalUserNum - The local player number the friends were read for
	 * @param ListName - named list to read
	 */
	void ReadFriendsUsingGraphPath(int32 LocalUserNum, const FString& ListName);

	/** Delegate used to notify this interface that permissions are updated, and that we can now read the friends list */
	FOnRequestNewReadPermissionsCompleteDelegate RequestFriendsReadPermissionsDelegate;


private:

	/** Reference to the main facebook identity */
	IOnlineIdentityPtr IdentityInterface;

	/** Reference to the main facebook sharing interface */
	IOnlineSharingPtr SharingInterface;

	/** The collection of facebook friends received through the fb callbacks in ReadFriendsList */
	TArray< TSharedRef<FOnlineFriendFacebook> > CachedFriends;

	/** Config based list of fields to use when querying friends list */
	TArray<FString> FriendsFields;
};

typedef TSharedPtr<FOnlineFriendsFacebook, ESPMode::ThreadSafe> FOnlineFriendsFacebookPtr;
