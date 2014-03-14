// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
 

// Module includes
#include "OnlineUserInterface.h"

/**
 * Info associated with an online user on the facebook service
 */
class FOnlineUserInfoFacebook : 
	public FOnlineUser
{
public:

	// FOnlineUser

	virtual TSharedRef<FUniqueNetId> GetUserId() const OVERRIDE;
	virtual FString GetRealName() const OVERRIDE;
	virtual FString GetDisplayName() const OVERRIDE;
	virtual bool GetUserAttribute(const FString& AttrName, FString& OutAttrValue) const OVERRIDE;

	// FOnlineUserInfoFacebook

	/**
	 * Init/default constructor
	 */
	FOnlineUserInfoFacebook(const FString& InUserId=TEXT("")) 
		: UserId(new FUniqueNetIdString(InUserId))
	{
	}

	/**
	 * Destructor
	 */
	virtual ~FOnlineUserInfoFacebook()
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
};


/**
 * Facebook implementation of the Online User Interface
 */
class FOnlineUserFacebook : public IOnlineUser
{

public:
	
	// IOnlineUser

	virtual bool QueryUserInfo(int32 LocalUserNum, const TArray<TSharedRef<class FUniqueNetId> >& UserIds) OVERRIDE;
	virtual bool GetAllUserInfo(int32 LocalUserNum, TArray< TSharedRef<class FOnlineUser> >& OutUsers) OVERRIDE;
	virtual TSharedPtr<FOnlineUser> GetUserInfo(int32 LocalUserNum, const class FUniqueNetId& UserId) OVERRIDE;

	// FOnlineUserFacebook

	/**
	 * Constructor used to indicate which OSS we are a part of
	 */
	FOnlineUserFacebook(class FOnlineSubsystemFacebook* InSubsystem);
	
	/**
	 * Default destructor
	 */
	virtual ~FOnlineUserFacebook();


private:

	/** Reference to the main IOS identity */
	class FOnlineIdentityFacebook* IdentityInterface;
	/** The collection of facebook users received through the fb callbacks in QueryUserInfo */
	TArray< TSharedRef<FOnlineUserInfoFacebook> > CachedUsers;
};


typedef TSharedPtr<FOnlineUserFacebook, ESPMode::ThreadSafe> FOnlineUserFacebookPtr;