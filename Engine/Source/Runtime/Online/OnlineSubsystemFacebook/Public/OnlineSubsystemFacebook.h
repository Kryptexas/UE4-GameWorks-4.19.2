// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineSubsystem.h"

/** Forward declarations of all interface classes */
typedef TSharedPtr<class FOnlineIdentityFacebook, ESPMode::ThreadSafe> FOnlineIdentityFacebookPtr;
typedef TSharedPtr<class FOnlineFriendsFacebook, ESPMode::ThreadSafe> FOnlineFriendsFacebookPtr;
typedef TSharedPtr<class FOnlineSharingFacebook, ESPMode::ThreadSafe> FOnlineSharingFacebookPtr;
typedef TSharedPtr<class FOnlineUserFacebook, ESPMode::ThreadSafe> FOnlineUserFacebookPtr;

/**
 *	OnlineSubsystemFacebook - Implementation of the online subsystem for Facebook services
 */
class ONLINESUBSYSTEMFACEBOOK_API FOnlineSubsystemFacebook 
	: public IOnlineSubsystem
	, public FTickerObjectBase
{
public:

	// IOnlineSubsystem Interface
	virtual IOnlineSessionPtr GetSessionInterface() const OVERRIDE;
	virtual IOnlineFriendsPtr GetFriendsInterface() const OVERRIDE;
	virtual IOnlineSharedCloudPtr GetSharedCloudInterface() const OVERRIDE;
	virtual IOnlineUserCloudPtr GetUserCloudInterface() const OVERRIDE;
	virtual IOnlineLeaderboardsPtr GetLeaderboardsInterface() const OVERRIDE;
	virtual IOnlineVoicePtr GetVoiceInterface() const OVERRIDE;
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

	// FTickerBaseObject

	virtual bool Tick(float DeltaTime) OVERRIDE;

	// FOnlineSubsystemFacebook

	/**
	 * Destructor
	 */
	virtual ~FOnlineSubsystemFacebook();

	/** 
	 * Create the singleton instance
	 *
	 * @return the only instance of this subsystem
	 */
	static FOnlineSubsystemFacebook* Create();

	/** 
	 * Destroy the singleton for this subsystem
	 */
	static void Destroy();

	/**
	 * Is Facebook available for use
	 * @return true if Facebook functionality is available, false otherwise
	 */
	bool IsEnabled();

private:

	/** 
	 * Private constructor as this is a singleton 
	 */
	FOnlineSubsystemFacebook();

	/** Single instantiation of the interface */
	static FOnlineSubsystemFacebook* FacebookSingleton;

	/** facebook implementation of identity interface */
	FOnlineIdentityFacebookPtr FacebookIdentity;

	/** facebook implementation of friends interface */
	FOnlineFriendsFacebookPtr FacebookFriends;

	/** facebook implementation of sharing interface */
	FOnlineSharingFacebookPtr FacebookSharing;

	/** facebook implementation of user interface */
	FOnlineUserFacebookPtr FacebookUser;
};

typedef TSharedPtr<FOnlineSubsystemFacebook, ESPMode::ThreadSafe> FOnlineSubsystemFacebookPtr;

