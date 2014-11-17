// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineSubsystem.h"
#include "OnlineIdentityInterfaceGooglePlay.h"
#include "OnlineAchievementsInterfaceGooglePlay.h"
#include "OnlineLeaderboardInterfaceGooglePlay.h"
#include "OnlineExternalUIInterfaceGooglePlay.h"

/**
 * OnlineSubsystemGooglePlay - Implementation of the online subsystem for Google Play services
 */
class ONLINESUBSYSTEMGOOGLEPLAY_API FOnlineSubsystemGooglePlay : 
	public IOnlineSubsystem,
	public FTickerObjectBase
{
public:
	FOnlineSubsystemGooglePlay();
	virtual ~FOnlineSubsystemGooglePlay() {}

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
	virtual IOnlineEventsPtr GetEventsInterface() const OVERRIDE { return NULL; }
	virtual IOnlineMessagePtr GetMessageInterface() const OVERRIDE { return NULL; }
	virtual IOnlineSharingPtr GetSharingInterface() const OVERRIDE { return NULL; }
	virtual IOnlineUserPtr GetUserInterface() const OVERRIDE { return NULL; }
	virtual IOnlineAchievementsPtr GetAchievementsInterface() const OVERRIDE;
	virtual IOnlinePresencePtr GetPresenceInterface() const OVERRIDE { return NULL; }
	virtual class UObject* GetNamedInterface(FName InterfaceName) OVERRIDE { return NULL; }
	virtual void SetNamedInterface(FName InterfaceName, class UObject* NewInterface) OVERRIDE {}
	virtual bool IsDedicated() const OVERRIDE { return false; }
	virtual bool IsServer() const OVERRIDE { return true; }
	virtual void SetForceDedicated(bool bForce) OVERRIDE {}
	virtual bool IsLocalPlayer(const FUniqueNetId& UniqueId) const OVERRIDE { return true; }

	virtual bool Init() OVERRIDE;
	virtual bool Shutdown() OVERRIDE;
	virtual FString GetAppId() const OVERRIDE;
	virtual bool Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) OVERRIDE;
	// End IOnlineSubsystem Interface

	virtual bool Tick(float DeltaTime) OVERRIDE;
	
PACKAGE_SCOPE:

	/**
	 * Is Online Subsystem Android available for use
	 * @return true if Android Online Subsystem functionality is available, false otherwise
	 */
	bool IsEnabled();

private:

	/** Online async task thread */
	class FRunnableThread* OnlineAsyncTaskThread;

	/** Interface to the online identity system */
	FOnlineIdentityGooglePlayPtr IdentityInterface;

	/** Interface to the online leaderboards */
	FOnlineLeaderboardsGooglePlayPtr LeaderboardsInterface;

	/** Interface to the online achievements */
	FOnlineAchievementsGooglePlayPtr AchievementsInterface;

	/** Interface to the external UI services */
	FOnlineExternalUIGooglePlayPtr ExternalUIInterface;
};

typedef TSharedPtr<FOnlineSubsystemGooglePlay, ESPMode::ThreadSafe> FOnlineSubsystemGooglePlayPtr;
