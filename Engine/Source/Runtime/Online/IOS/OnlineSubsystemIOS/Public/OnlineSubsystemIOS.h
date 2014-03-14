// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineSubsystem.h"


/**
 *	OnlineSubsystemIOS - Implementation of the online subsystem for IOS services
 */
class ONLINESUBSYSTEMIOS_API FOnlineSubsystemIOS : 
	public IOnlineSubsystem,
	public FTickerObjectBase
{
protected:
	/** Single instantiation of the IOS interface */
	static FOnlineSubsystemIOS* IOSSingleton;

public:
	FOnlineSubsystemIOS();
	virtual ~FOnlineSubsystemIOS() {};

	// Begin IOnlineSubsystem Interface
	virtual IOnlineSessionPtr GetSessionInterface() const OVERRIDE;
	virtual IOnlineFriendsPtr GetFriendsInterface() const OVERRIDE;
	virtual IOnlineSharedCloudPtr GetSharedCloudInterface() const OVERRIDE;
	virtual IOnlineUserCloudPtr GetUserCloudInterface() const OVERRIDE;
	virtual IOnlineLeaderboardsPtr GetLeaderboardsInterface() const OVERRIDE;
	virtual IOnlineVoicePtr GetVoiceInterface() const  OVERRIDE;
	virtual IOnlineExternalUIPtr GetExternalUIInterface() const OVERRIDE;
	virtual IOnlineTimePtr GetTimeInterface() const OVERRIDE;
	virtual IOnlineIdentityPtr GetIdentityInterface() const OVERRIDE;
	virtual IOnlineTitleFilePtr GetTitleFileInterface() const OVERRIDE;
	virtual IOnlineEntitlementsPtr GetEntitlementsInterface() const OVERRIDE;
	virtual IOnlineStorePtr GetStoreInterface() const OVERRIDE;
	virtual IOnlineEventsPtr GetEventsInterface() const OVERRIDE;
	virtual IOnlineAchievementsPtr GetAchievementsInterface() const OVERRIDE;
	virtual IOnlineSharingPtr GetSharingInterface() const OVERRIDE;
	virtual IOnlineUserPtr GetUserInterface() const OVERRIDE;
	virtual IOnlineMessagePtr GetMessageInterface() const OVERRIDE;
	virtual IOnlinePresencePtr GetPresenceInterface() const OVERRIDE;
	virtual bool Init() OVERRIDE;
	virtual bool Shutdown() OVERRIDE;
	virtual FString GetAppId() const OVERRIDE;
	virtual bool Exec(const TCHAR* Cmd, FOutputDevice& Ar) OVERRIDE;
	virtual bool Tick(float DeltaTime) OVERRIDE;
	// End IOnlineSubsystem Interface

PACKAGE_SCOPE:

	/** 
	 * Singleton interface for the IOS subsystem 
	 * @return the only instance of the IOS subsystem
	 */
	static FOnlineSubsystemIOS* Create();

	/** 
	 * Destroy the singleton IOS subsystem
	 */
	static void Destroy();

	/**
	 * Is IOS available for use
	 * @return true if IOS is available, false otherwise
	 */
	bool IsEnabled();

private:

	/** Online async task thread */
	class FRunnableThread* OnlineAsyncTaskThread;

	/** Interface to the session services */
	FOnlineSessionIOSPtr SessionInterface;

	/** Interface to the Identity information */
	FOnlineIdentityIOSPtr IdentityInterface;

	/** Interface to the friends services */
	FOnlineFriendsIOSPtr FriendsInterface;

	/** Interface to the profile information */
	FOnlineLeaderboardsIOSPtr LeaderboardsInterface;

	/** Interface to the online store */
	FOnlineStoreInterfaceIOSPtr StoreInterface;

	/** Interface to the online achievements */
	FOnlineAchievementsIOSPtr AchievementsInterface;
};

typedef TSharedPtr<FOnlineSubsystemIOS, ESPMode::ThreadSafe> FOnlineSubsystemIOSPtr;

