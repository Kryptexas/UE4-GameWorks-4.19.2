// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineFriendsInterface.h"
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

	/**
	 * Constructor
	 *
	 * @param InSubsystem Facebook subsystem being used
	 */
	FOnlineFriendsFacebook(class FOnlineSubsystemFacebook* InSubsystem);
	
	/**
	 * Destructor
	 */
	virtual ~FOnlineFriendsFacebook();

private:

	/**
	 * Should use the initialization constructor instead
	 */
	FOnlineFriendsFacebook();

	/**
	 * Delegate called when a user /me request from facebook is complete
	 */
	void QueryFriendsList_HttpRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);

	/** For accessing identity/token info of user logged in */
	FOnlineSubsystemFacebook* FacebookSubsystem;
	/** Config based url for querying friends list */
	FString FriendsUrl;
	/** Config based list of fields to use when querying friends list */
	TArray<FString> FriendsFields;

	/** List of online friends */
	struct FOnlineFriendsList
	{
		TArray< TSharedRef<FOnlineFriendFacebook> > Friends;
	};
	/** Cached friends list from last call to ReadFriendsList for each local user */
	TMap<int, FOnlineFriendsList> FriendsMap;

	/** Info used to send request to register a user */
	struct FPendingFriendsQuery
	{
		FPendingFriendsQuery(int32 InLocalUserNum=0) 
			: LocalUserNum(InLocalUserNum)
		{
		}
		/** local index of user making the request */
		int32 LocalUserNum;
	};
	/** List of pending Http requests for user registration */
	TMap<class IHttpRequest*, FPendingFriendsQuery> FriendsQueryRequests;
};

typedef TSharedPtr<FOnlineFriendsFacebook, ESPMode::ThreadSafe> FOnlineFriendsFacebookPtr;
