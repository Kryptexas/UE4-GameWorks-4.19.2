// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
 

// Module includes
#include "OnlineIdentityInterface.h"


// Facebook SDK includes
#include "FBSession.h"
#include "FBGraphUser.h"
#include "FBRequest.h"
#include "FBRequestConnection.h"
#include "FBAccessTokenData.h"

/**
 * Facebook implementation of the online account information we may want
 */
class FUserOnlineAccountFacebook : public FUserOnlineAccount
{

public:

	// FOnlineUser
	
	virtual TSharedRef<FUniqueNetId> GetUserId() const OVERRIDE;
	virtual FString GetRealName() const OVERRIDE;
	virtual FString GetDisplayName() const OVERRIDE;
	virtual bool GetUserAttribute(const FString& AttrName, FString& OutAttrValue) const OVERRIDE;

	// FUserOnlineAccount

	virtual FString GetAccessToken() const OVERRIDE;
	virtual bool GetAuthAttribute(const FString& AttrName, FString& OutAttrValue) const OVERRIDE;

public:

	/**
	 * Constructor
	 *
	 * @param	InUserId		The user Id which should be assigned for this account.
	 * @param	InAuthTicket	The token which this account is using.
	 */
	FUserOnlineAccountFacebook(const FString& InUserId=TEXT(""), const FString& InAuthTicket=TEXT("")) ;

	/**
	 * Destructor
	 */
	virtual ~FUserOnlineAccountFacebook(){}


private:

	/** The token obtained for this session */
	FString AuthTicket;

	/** UID of the logged in user */
	TSharedRef<FUniqueNetId> UserId;

	/** Allow the fb identity to fill in our private members from it's callbacks */
	friend class FOnlineIdentityFacebook;
};


/**
 * Facebook service implementation of the online identity interface
 */
class FOnlineIdentityFacebook : public IOnlineIdentity
{

public:

	// Begin IOnlineIdentity interface	
	virtual bool Login(int32 LocalUserNum, const FOnlineAccountCredentials& AccountCredentials) OVERRIDE;
	virtual bool Logout(int32 LocalUserNum) OVERRIDE;
	virtual bool AutoLogin(int32 LocalUserNum) OVERRIDE;
	virtual TSharedPtr<FUserOnlineAccount> GetUserAccount(const FUniqueNetId& UserId) const OVERRIDE;
	virtual TArray<TSharedPtr<FUserOnlineAccount> > GetAllUserAccounts() const OVERRIDE;
	virtual TSharedPtr<FUniqueNetId> GetUniquePlayerId(int32 LocalUserNum) const OVERRIDE;
	virtual TSharedPtr<FUniqueNetId> CreateUniquePlayerId(uint8* Bytes, int32 Size) OVERRIDE;
	virtual TSharedPtr<FUniqueNetId> CreateUniquePlayerId(const FString& Str) OVERRIDE;
	virtual ELoginStatus::Type GetLoginStatus(int32 LocalUserNum) const OVERRIDE;
	virtual FString GetPlayerNickname(int32 LocalUserNum) const OVERRIDE;
	virtual FString GetAuthToken(int32 LocalUserNum) const OVERRIDE;
	// End IOnlineIdentity interface


public:

	/**
	 * Default constructor
	 */
	FOnlineIdentityFacebook();


	/**
	 * Destructor
	 */
	virtual ~FOnlineIdentityFacebook()
	{
	}


private:

	/** Access for the current FB Session if logged in */
	FBSession* CurrentSession;

	/** The user account details associated with this identity */
	TSharedRef< FUserOnlineAccountFacebook > UserAccount;

	/** The current state of our login */
	ELoginStatus::Type LoginStatus;

	/** Username of the user assuming this identity */
	FString Name;
};

typedef TSharedPtr<FOnlineIdentityFacebook, ESPMode::ThreadSafe> FOnlineIdentityFacebookPtr;