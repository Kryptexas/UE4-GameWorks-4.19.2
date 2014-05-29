// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineSubsystem.h"
#include "OnlineSubsystemImpl.h"
#include "OnlineSubsystemAmazonModule.h"
#include "OnlineSubsystemAmazonPackage.h"

/** Forward declarations of all interface classes */
typedef TSharedPtr<class FOnlineIdentityAmazon, ESPMode::ThreadSafe> FOnlineIdentityAmazonPtr;

/**
 * Amazon subsystem
 */
class ONLINESUBSYSTEMAMAZON_API FOnlineSubsystemAmazon :
	public FOnlineSubsystemImpl,
	public FTickerObjectBase

{
	class FOnlineFactoryAmazon* AmazonFactory;

	/** Interface to the identity registration/auth services */
	FOnlineIdentityAmazonPtr IdentityInterface;

	/** Used to toggle between 1 and 0 */
	int TickToggle;

PACKAGE_SCOPE:

	/** Only the factory makes instances */
	FOnlineSubsystemAmazon();

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
	virtual IOnlinePartyPtr GetPartyInterface() const OVERRIDE
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

	virtual IOnlinePartyPtr GetPartyInterface() const OVERRIDE
	{
		return NULL;
	}

	virtual bool Init() OVERRIDE;
	virtual bool Shutdown() OVERRIDE;
	virtual FString GetAppId() const OVERRIDE;
	virtual bool Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) OVERRIDE;

	// FTickerBaseObject

	virtual bool Tick(float DeltaTime) OVERRIDE;

	// FOnlineSubsystemAmazon

	/**
	 * Destructor
	 */
	virtual ~FOnlineSubsystemAmazon();

	/**
	 * @return whether this subsystem is enabled or not
	 */
	bool IsEnabled();
};

typedef TSharedPtr<FOnlineSubsystemAmazon, ESPMode::ThreadSafe> FOnlineSubsystemAmazonPtr;