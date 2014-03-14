// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once


#include "OnlineSubsystemIOSTypes.h"
#include "OnlineIdentityInterface.h"
#include "OnlineSubsystemIOSPackage.h"


class FOnlineIdentityIOS :
	public IOnlineIdentity
{
private:
	/** UID for this identity */
	TSharedPtr< FUniqueNetIdString > UniqueNetId;


PACKAGE_SCOPE:

	/**
	 * Default Constructor
	 */
	FOnlineIdentityIOS();	


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
	 * Destructor
	 */
	virtual ~FOnlineIdentityIOS() {};
	

	/**
	 * Get a reference to the GKLocalPlayer
	 *
	 * @return - The game center local player
	 */
	GKLocalPlayer* GetLocalGameCenterUser() const
	{
		return [GKLocalPlayer localPlayer];
	}
};


typedef TSharedPtr<FOnlineIdentityIOS, ESPMode::ThreadSafe> FOnlineIdentityIOSPtr;