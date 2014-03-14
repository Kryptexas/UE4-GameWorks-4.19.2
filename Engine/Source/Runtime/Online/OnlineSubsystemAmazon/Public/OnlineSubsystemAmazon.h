// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineSubsystem.h"
#include "OnlineSubsystemAmazonModule.h"

/** Forward declarations of all interface classes */
typedef TSharedPtr<class FOnlineIdentityAmazon, ESPMode::ThreadSafe> FOnlineIdentityAmazonPtr;

/**
 * Amazon subsystem
 */
class ONLINESUBSYSTEMAMAZON_API FOnlineSubsystemAmazon :
	public IOnlineSubsystem,
	public FTickerObjectBase

{
	/** 
	 * Private constructor as this is a singleton 
	 */
	FOnlineSubsystemAmazon();

	/** Single instantiation of the interface */
	static FOnlineSubsystemAmazon* AmazonSingleton;

	/** Interface to the identity registration/auth services */
	FOnlineIdentityAmazonPtr IdentityInterface;

	/** Used to toggle between 1 and 0 */
	int TickToggle;

public:
	// IOnlineSubsystem

	virtual IOnlineSessionPtr GetSessionInterface() const OVERRIDE
	{
		return NULL;
	}
	virtual IOnlineFriendsPtr GetFriendsInterface() const OVERRIDE
	{
		return NULL;
	}
	virtual IOnlineSharedCloudPtr GetSharedCloudInterface() const OVERRIDE
	{
		return NULL;
	}
	virtual IOnlineUserCloudPtr GetUserCloudInterface() const OVERRIDE
	{
		return NULL;
	}
	virtual IOnlineLeaderboardsPtr GetLeaderboardsInterface() const OVERRIDE
	{
		return NULL;
	}
	virtual IOnlineVoicePtr GetVoiceInterface() const OVERRIDE
	{
		return NULL;
	}
	virtual IOnlineExternalUIPtr GetExternalUIInterface() const OVERRIDE	
	{
		return NULL;
	}
	virtual IOnlineTimePtr GetTimeInterface() const OVERRIDE
	{
		return NULL;
	}

	virtual IOnlineTitleFilePtr GetTitleFileInterface() const OVERRIDE
	{
		return NULL;
	}

	virtual IOnlineEntitlementsPtr GetEntitlementsInterface() const OVERRIDE
	{
		return NULL;
	}

	virtual IOnlineIdentityPtr GetIdentityInterface() const OVERRIDE;

	virtual IOnlineStorePtr GetStoreInterface() const OVERRIDE
	{
		return NULL;
	}

	virtual IOnlineEventsPtr GetEventsInterface() const OVERRIDE
	{
		return NULL;
	}

	virtual IOnlineAchievementsPtr GetAchievementsInterface() const OVERRIDE
	{
		return NULL;
	}

	virtual IOnlineSharingPtr GetSharingInterface() const OVERRIDE
	{
		return NULL;
	}

	virtual IOnlineUserPtr GetUserInterface() const OVERRIDE
	{
		return NULL;
	}

	virtual IOnlineMessagePtr GetMessageInterface() const OVERRIDE
	{
		return NULL;
	}

	virtual IOnlinePresencePtr GetPresenceInterface() const OVERRIDE
	{
		return NULL;
	}

	virtual bool Init() OVERRIDE;
	virtual bool Shutdown() OVERRIDE;
	virtual FString GetAppId() const OVERRIDE;
	virtual bool Exec(const TCHAR* Cmd, FOutputDevice& Ar) OVERRIDE;

	// FTickerBaseObject

	virtual bool Tick(float DeltaTime) OVERRIDE;

	// FOnlineSubsystemAmazon

	/**
	 * Destructor
	 */
	virtual ~FOnlineSubsystemAmazon();

	/** 
	 * Create the singleton instance
	 *
	 * @return the only instance of this subsystem
	 */
	static FOnlineSubsystemAmazon* Create();

	/** 
	 * Destroy the singleton for this subsystem
	 */
	static void Destroy();
	/**
	 * @return whether this subsystem is enabled or not
	 */
	bool IsEnabled();
};


