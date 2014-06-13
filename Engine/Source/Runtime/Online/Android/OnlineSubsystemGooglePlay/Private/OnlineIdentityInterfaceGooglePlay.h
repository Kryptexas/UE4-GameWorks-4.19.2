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
	virtual TSharedPtr<FUserOnlineAccount> GetUserAccount(const FUniqueNetId& UserId) const override;
	virtual TArray<TSharedPtr<FUserOnlineAccount> > GetAllUserAccounts() const override;
	virtual bool Login(int32 LocalUserNum, const FOnlineAccountCredentials& AccountCredentials) override;
	virtual bool Logout(int32 LocalUserNum) override;
	virtual bool AutoLogin(int32 LocalUserNum) override;
	virtual TSharedPtr<FUniqueNetId> GetUniquePlayerId(int32 LocalUserNum) const override;
	virtual TSharedPtr<FUniqueNetId> CreateUniquePlayerId(uint8* Bytes, int32 Size) override;
	virtual TSharedPtr<FUniqueNetId> CreateUniquePlayerId(const FString& Str) override;
	virtual ELoginStatus::Type GetLoginStatus(int32 LocalUserNum) const override;
	virtual FString GetPlayerNickname(int32 LocalUserNum) const override;
	virtual FString GetAuthToken(int32 LocalUserNum) const override;
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

