// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineSubsystem.h"

/** Forward declarations of all interface classes */
typedef TSharedPtr<class FOnlineSessionNull, ESPMode::ThreadSafe> FOnlineSessionNullPtr;
typedef TSharedPtr<class FOnlineProfileNull, ESPMode::ThreadSafe> FOnlineProfileNullPtr;
typedef TSharedPtr<class FOnlineFriendsNull, ESPMode::ThreadSafe> FOnlineFriendsNullPtr;
typedef TSharedPtr<class FOnlineUserCloudNull, ESPMode::ThreadSafe> FOnlineUserCloudNullPtr;
typedef TSharedPtr<class FOnlineLeaderboardsNull, ESPMode::ThreadSafe> FOnlineLeaderboardsNullPtr;
typedef TSharedPtr<class FOnlineVoiceNull, ESPMode::ThreadSafe> FOnlineVoiceNullPtr;
typedef TSharedPtr<class FOnlineExternalUINull, ESPMode::ThreadSafe> FOnlineExternalUINullPtr;
typedef TSharedPtr<class FOnlineIdentityNull, ESPMode::ThreadSafe> FOnlineIdentityNullPtr;
typedef TSharedPtr<class FOnlineAchievementsNull, ESPMode::ThreadSafe> FOnlineAchievementsNullPtr;

/**
 *	OnlineSubsystemNull - Implementation of the online subsystem for Null services
 */
class ONLINESUBSYSTEMNULL_API FOnlineSubsystemNull : 
	public IOnlineSubsystem,
	public FTickerObjectBase
{

public:

	virtual ~FOnlineSubsystemNull()
	{
	}

	// IOnlineSubsystem

	virtual IOnlineSessionPtr GetSessionInterface() const OVERRIDE;
	virtual IOnlineFriendsPtr GetFriendsInterface() const OVERRIDE;
	virtual IOnlineSharedCloudPtr GetSharedCloudInterface() const OVERRIDE;
	virtual IOnlineUserCloudPtr GetUserCloudInterface() const OVERRIDE;
	virtual IOnlineEntitlementsPtr GetEntitlementsInterface() const OVERRIDE;
	virtual IOnlineLeaderboardsPtr GetLeaderboardsInterface() const OVERRIDE;
	virtual IOnlineVoicePtr GetVoiceInterface() const OVERRIDE;
	virtual IOnlineExternalUIPtr GetExternalUIInterface() const OVERRIDE;	
	virtual IOnlineTimePtr GetTimeInterface() const OVERRIDE;
	virtual IOnlineIdentityPtr GetIdentityInterface() const OVERRIDE;
	virtual IOnlineTitleFilePtr GetTitleFileInterface() const OVERRIDE;
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

	// FTickerObjectBase
	
	virtual bool Tick(float DeltaTime) OVERRIDE;

	// FOnlineSubsystemNull

	/** 
	 * Singleton interface for the Null subsystem 
	 * @return the only instance of the Null subsystem
	 */
	static FOnlineSubsystemNull* Create();

	/** 
	 * Destroy the singleton Null subsystem
	 */
	static void Destroy();

	/**
	 * Is the Null API available for use
	 * @return true if Null functionality is available, false otherwise
	 */
	bool IsEnabled();

private:

	/** Single instantiation of the Null interface */
	static FOnlineSubsystemNull* NullSingleton;

	/** Interface to the session services */
	FOnlineSessionNullPtr SessionInterface;

	/** Interface to the leaderboard services */
	FOnlineLeaderboardsNullPtr LeaderboardsInterface;

	/** Interface to the identity registration/auth services */
	FOnlineIdentityNullPtr IdentityInterface;

	/** Interface for achievements */
	FOnlineAchievementsNullPtr AchievementsInterface;

	/** Online async task runnable */
	class FOnlineAsyncTaskManagerNull* OnlineAsyncTaskThreadRunnable;

	/** Online async task thread */
	class FRunnableThread* OnlineAsyncTaskThread;

	/** Private constructor as this is a singleton */
	FOnlineSubsystemNull()
	: SessionInterface(NULL)
	, LeaderboardsInterface(NULL)
	, IdentityInterface(NULL)
	, AchievementsInterface(NULL)
	, OnlineAsyncTaskThreadRunnable(NULL)
	, OnlineAsyncTaskThread(NULL)
	{}

};

typedef TSharedPtr<FOnlineSubsystemNull, ESPMode::ThreadSafe> FOnlineSubsystemNullPtr;

