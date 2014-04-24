// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineIdentityInterface.h"
#include "OnlineSubsystemGooglePlayPackage.h"

class FOnlineIdentityGooglePlay :
	public IOnlineIdentity
{
private:
	
	bool bPrevLoggedIn;
	bool bLoggedIn;
	FString PlayerAlias;
	int32 CurrentLoginUserNum;
	class FOnlineSubsystemGooglePlay* MainSubsystem;

	bool bLoggingInUser;
	bool bRegisteringUser;

PACKAGE_SCOPE:

	/**
	 * Default Constructor
	 */
	FOnlineIdentityGooglePlay(FOnlineSubsystemGooglePlay* InSubsystem);

public:

	// Begin IOnlineIdentity interface
	virtual TSharedPtr<FUserOnlineAccount> GetUserAccount(const FUniqueNetId& UserId) const OVERRIDE;
	virtual TArray<TSharedPtr<FUserOnlineAccount> > GetAllUserAccounts() const OVERRIDE;
	virtual bool Login(int32 LocalUserNum, const FOnlineAccountCredentials& AccountCredentials) OVERRIDE;
	virtual bool Logout(int32 LocalUserNum) OVERRIDE;
	virtual bool AutoLogin(int32 LocalUserNum) OVERRIDE;
	virtual TSharedPtr<FUniqueNetId> GetUniquePlayerId(int32 LocalUserNum) const OVERRIDE;
	virtual TSharedPtr<FUniqueNetId> CreateUniquePlayerId(uint8* Bytes, int32 Size) OVERRIDE;
	virtual TSharedPtr<FUniqueNetId> CreateUniquePlayerId(const FString& Str) OVERRIDE;
	virtual ELoginStatus::Type GetLoginStatus(int32 LocalUserNum) const OVERRIDE;
	virtual FString GetPlayerNickname(int32 LocalUserNum) const OVERRIDE;
	virtual FString GetAuthToken(int32 LocalUserNum) const OVERRIDE;
	// End IOnlineIdentity interface

public:

	// Begin IOnlineIdentity interface
	void Tick(float DeltaTime);
	// End IOnlineIdentity interface
	
	//platform specific
	void OnLogin(const bool InLoggedIn,  const FString& InPlayerId, const FString& InPlayerAlias);
	void OnLogout(const bool InLoggedIn);

};


typedef TSharedPtr<FOnlineIdentityGooglePlay, ESPMode::ThreadSafe> FOnlineIdentityGooglePlayPtr;

