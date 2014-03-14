// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineSubsystemSteamTypes.h"
#include "OnlineIdentityInterface.h"
#include "OnlineSubsystemSteamPackage.h"

class FOnlineIdentitySteam :
	public IOnlineIdentity
{
	/** The steam user interface to use when interacting with steam */
	class ISteamUser* SteamUserPtr;
	/** The steam friends interface to use when interacting with steam */
	class ISteamFriends* SteamFriendsPtr;

PACKAGE_SCOPE:

	FOnlineIdentitySteam();	

public:

	virtual ~FOnlineIdentitySteam() {};

	// IOnlineIdentity

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
};

typedef TSharedPtr<FOnlineIdentitySteam, ESPMode::ThreadSafe> FOnlineIdentitySteamPtr;