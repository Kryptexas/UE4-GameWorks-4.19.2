// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineIdentityInterface.h"
#include "OnlineSubsystemGooglePlayPackage.h"

extern "C" void Java_com_epicgames_ue4_GameActivity_nativeCompletedConnection(JNIEnv* LocalJNIEnv, jobject LocalThiz, jint userID, jint errorCode);

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

	/** UID for this identity */
	TSharedPtr< FUniqueNetIdString > UniqueNetId;

	struct FPendingConnection
	{
		FOnlineIdentityGooglePlay * ConnectionInterface;
		bool IsConnectionPending;
	};

	/** Instance of the connection context data */
	static FPendingConnection PendingConnectRequest;

	/**
	* Native function called from Java when we get the connection result callback.
	*
	* @param LocalJNIEnv the current JNI environment
	* @param LocalThiz instance of the Java GameActivity class
	* @param errorCode The reported error code from the connection attempt
	*/
	friend void Java_com_epicgames_ue4_GameActivity_nativeCompletedConnection(JNIEnv* LocalJNIEnv, jobject LocalThiz, jint userID, jint errorCode);

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
	virtual ELoginStatus::Type GetLoginStatus(const FUniqueNetId& UserId) const override;
	virtual FString GetPlayerNickname(int32 LocalUserNum) const override;
	virtual FString GetPlayerNickname(const FUniqueNetId& UserId) const override;
	virtual FString GetAuthToken(int32 LocalUserNum) const override;
	// End IOnlineIdentity interface

public:

	// Begin IOnlineIdentity interface
	void Tick(float DeltaTime);
	// End IOnlineIdentity interface
	
	//platform specific
	void OnLogin(const bool InLoggedIn,  const FString& InPlayerId, const FString& InPlayerAlias);
	void OnLogout(const bool InLoggedIn);

	void OnLoginCompleted(const int playerID, const int errorCode);

};


typedef TSharedPtr<FOnlineIdentityGooglePlay, ESPMode::ThreadSafe> FOnlineIdentityGooglePlayPtr;

