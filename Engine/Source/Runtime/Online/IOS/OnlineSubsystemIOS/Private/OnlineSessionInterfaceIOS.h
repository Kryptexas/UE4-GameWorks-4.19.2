// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineSessionInterface.h"
#include "OnlineSubsystemIOSTypes.h"

#include <GameKit/GKSession.h>
#include <GameKit/GKLocalPlayer.h>


@interface FGameCenterSessionDelegate : UIViewController<GKSessionDelegate>
{
};

@property (nonatomic, strong) GKSession *Session;

-(void) initSessionWithName:(NSString*) sessionName;
-(void) shutdownSession;

@end


/**
 * Interface definition for the online services session services 
 * Session services are defined as anything related managing a session 
 * and its state within a platform service
 */
class FOnlineSessionIOS : public IOnlineSession
{
private:

	/** Reference to the main GameCenter subsystem */
	class FOnlineSubsystemIOS* IOSSubsystem;

	/** Hidden on purpose */
	FOnlineSessionIOS();

PACKAGE_SCOPE:

	/** Critical sections for thread safe operation of session lists */
	mutable FCriticalSection SessionLock;

	/** Current session settings */
	TArray<FNamedOnlineSession> Sessions;

	TMap< FName, FGameCenterSessionDelegate* > GKSessions;

	/** Current search object */
	TSharedPtr<FOnlineSessionSearch> CurrentSessionSearch;


PACKAGE_SCOPE:

	/** Constructor */
	FOnlineSessionIOS(class FOnlineSubsystemIOS* InSubsystem);

	/**
	 * Session tick for various background tasks
	 */
	void Tick(float DeltaTime);


	// Begin IOnlineSession interface
	class FNamedOnlineSession* AddNamedSession(FName SessionName, const FOnlineSessionSettings& SessionSettings) OVERRIDE;

	class FNamedOnlineSession* AddNamedSession(FName SessionName, const FOnlineSession& Session) OVERRIDE;

public:

	virtual ~FOnlineSessionIOS();

	FNamedOnlineSession* GetNamedSession(FName SessionName) OVERRIDE;

	virtual void RemoveNamedSession(FName SessionName) OVERRIDE;

	virtual EOnlineSessionState::Type GetSessionState(FName SessionName) const OVERRIDE;

	virtual bool HasPresenceSession() OVERRIDE;

	virtual bool CreateSession(int32 HostingPlayerNum, FName SessionName, const FOnlineSessionSettings& NewSessionSettings) OVERRIDE;

	virtual bool StartSession(FName SessionName) OVERRIDE;

	virtual bool UpdateSession(FName SessionName, FOnlineSessionSettings& UpdatedSessionSettings, bool bShouldRefreshOnlineData = false) OVERRIDE;

	virtual bool EndSession(FName SessionName) OVERRIDE;

	virtual bool DestroySession(FName SessionName) OVERRIDE;

	virtual bool IsPlayerInSession(FName SessionName, const FUniqueNetId& UniqueId) OVERRIDE;

	virtual bool FindSessions(int32 SearchingPlayerNum, const TSharedRef<FOnlineSessionSearch>& SearchSettings) OVERRIDE;

	virtual bool CancelFindSessions() OVERRIDE;

	virtual bool PingSearchResults(const FOnlineSessionSearchResult& SearchResult) OVERRIDE;

	virtual bool JoinSession(int32 PlayerNum, FName SessionName, const FOnlineSessionSearchResult& DesiredSession) OVERRIDE;

	virtual bool FindFriendSession(int32 LocalUserNum, const FUniqueNetId& Friend) OVERRIDE;

	virtual bool SendSessionInviteToFriend(int32 LocalUserNum, FName SessionName, const FUniqueNetId& Friend) OVERRIDE;

	virtual bool SendSessionInviteToFriends(int32 LocalUserNum, FName SessionName, const TArray< TSharedRef<FUniqueNetId> >& Friends) OVERRIDE;

	virtual bool GetResolvedConnectString(FName SessionName, FString& ConnectInfo) OVERRIDE;

	virtual bool GetResolvedConnectString(const class FOnlineSessionSearchResult& SearchResult, FName PortType, FString& ConnectInfo) OVERRIDE;

	virtual FOnlineSessionSettings* GetSessionSettings(FName SessionName) OVERRIDE;

	virtual bool RegisterPlayer(FName SessionName, const FUniqueNetId& PlayerId, bool bWasInvited) OVERRIDE;

	virtual bool RegisterPlayers(FName SessionName, const TArray< TSharedRef<FUniqueNetId> >& Players, bool bWasInvited = false) OVERRIDE;

	virtual bool UnregisterPlayer(FName SessionName, const FUniqueNetId& PlayerId) OVERRIDE;

	virtual bool UnregisterPlayers(FName SessionName, const TArray< TSharedRef<FUniqueNetId> >& Players) OVERRIDE;

	virtual int32 GetNumSessions() OVERRIDE;

	virtual void DumpSessionState() OVERRIDE;
	// End IOnlineSession interface
};

typedef TSharedPtr<FOnlineSessionIOS, ESPMode::ThreadSafe> FOnlineSessionIOSPtr;
