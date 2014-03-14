// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemIOSPrivatePCH.h"

@implementation FGameCenterSessionDelegate
@synthesize Session;

- (void)initSessionWithName:(NSString*) sessionName
{
	UE_LOG(LogOnline, Display, TEXT("- (void)initSessionWithName:(NSString*) sessionName"));
    self.Session = [[GKSession alloc] initWithSessionID:sessionName displayName:nil sessionMode:GKSessionModePeer];
    self.Session.delegate = self;
    self.Session.disconnectTimeout = 5.0f;
    self.Session.available = YES;
}

- (void)shutdownSession
{
	UE_LOG(LogOnline, Display, TEXT("- (void)shutdownSession"));
	[self.Session disconnectFromAllPeers];
	self.Session.available = NO;
	self.Session.delegate = nil;
}

-(void)session:(GKSession *)session didReceiveConnectionRequestFromPeer:(NSString *)peerID
{
	UE_LOG(LogOnline, Display, TEXT("-(void)session:(GKSession *)session didReceiveConnectionRequestFromPeer:(NSString *)peerID - %s"), ANSI_TO_TCHAR([[session displayNameForPeer:peerID] cStringUsingEncoding:NSASCIIStringEncoding]));
	[session acceptConnectionFromPeer:peerID error:nil];
}

- (void)connectToPeer:(NSString *)peerId 
{
    [self.Session connectToPeer:peerId withTimeout:10];
}

- (void)session:(GKSession *)session peer:(NSString *)peerID didChangeState:(GKPeerConnectionState)state
{
	switch (state)
	{
		case GKPeerStateAvailable:
		{
			UE_LOG(LogOnline, Display, TEXT("Peer available: %s"), ANSI_TO_TCHAR([[session displayNameForPeer:peerID] cStringUsingEncoding:NSASCIIStringEncoding]));

			[session connectToPeer:peerID withTimeout:5.0f];
			break;
		}

		case GKPeerStateUnavailable:
		{
			UE_LOG(LogOnline, Display, TEXT("Peer unavailable: %s"), ANSI_TO_TCHAR([[session displayNameForPeer:peerID] cStringUsingEncoding:NSASCIIStringEncoding]));
			break;
		}

		case GKPeerStateConnected:
		{
			UE_LOG(LogOnline, Display, TEXT("Peer connected: %s"), ANSI_TO_TCHAR([[session displayNameForPeer:peerID] cStringUsingEncoding:NSASCIIStringEncoding]));
			break;
		}

		case GKPeerStateDisconnected:
		{
			UE_LOG(LogOnline, Display, TEXT("Peer disconnected: %s"), ANSI_TO_TCHAR([[session displayNameForPeer:peerID] cStringUsingEncoding:NSASCIIStringEncoding]));
			break;
		}

		case GKPeerStateConnecting:
		{
			UE_LOG(LogOnline, Display, TEXT("Peer connecting: %s"), ANSI_TO_TCHAR([[session displayNameForPeer:peerID] cStringUsingEncoding:NSASCIIStringEncoding]));
			break;
		}
	}
}


- (void)session:(GKSession *)session didFailWithError:(NSError *)error
{
	UE_LOG(LogOnline, Warning, TEXT("Session failed with error code %i"), [error code]);

	[self shutdownSession];
}


- (void)session:(GKSession *)session connectionWithPeerFailed:(NSString *)peerID withError:(NSError *)error
{
	NSString* ErrorString = [NSString stringWithFormat:@"connectionWithPeerFailed - Failed to connect to %@ with error code %i", [session displayNameForPeer:peerID], [error code]];
	FString ConvertedErrorStr = ANSI_TO_TCHAR([ErrorString cStringUsingEncoding:NSASCIIStringEncoding]);
	UE_LOG(LogOnline, Display, TEXT("%s"), ANSI_TO_TCHAR([ErrorString cStringUsingEncoding:NSASCIIStringEncoding]) );
}

@end


FOnlineSessionIOS::FOnlineSessionIOS() :
	IOSSubsystem(NULL),
	CurrentSessionSearch(NULL)
{
	UE_LOG(LogOnline, Display, TEXT("FOnlineSessionIOS::FOnlineSessionIOS()"));
}


FOnlineSessionIOS::FOnlineSessionIOS(FOnlineSubsystemIOS* InSubsystem) :
	IOSSubsystem(InSubsystem),
	CurrentSessionSearch(NULL)
{
	UE_LOG(LogOnline, Display, TEXT("FOnlineSessionIOS::FOnlineSessionIOS(FOnlineSubsystemIOS* InSubsystem)"));
}


FOnlineSessionIOS::~FOnlineSessionIOS()
{

}


void FOnlineSessionIOS::Tick(float DeltaTime)
{

}


FNamedOnlineSession* FOnlineSessionIOS::AddNamedSession(FName SessionName, const FOnlineSessionSettings& SessionSettings)
{
	FScopeLock ScopeLock(&SessionLock);
	return new (Sessions) FNamedOnlineSession(SessionName, SessionSettings);
}


FNamedOnlineSession* FOnlineSessionIOS::AddNamedSession(FName SessionName, const FOnlineSession& Session)
{
	FScopeLock ScopeLock(&SessionLock);
	return new (Sessions) FNamedOnlineSession(SessionName, Session);
}


FNamedOnlineSession* FOnlineSessionIOS::GetNamedSession(FName SessionName)
{
	FScopeLock ScopeLock(&SessionLock);
	for (int32 SearchIndex = 0; SearchIndex < Sessions.Num(); SearchIndex++)
	{
		if (Sessions[SearchIndex].SessionName == SessionName)
		{
			return &Sessions[SearchIndex];
		}
	}
	return NULL;
}


void FOnlineSessionIOS::RemoveNamedSession(FName SessionName)
{
	FScopeLock ScopeLock(&SessionLock);
	for (int32 SearchIndex = 0; SearchIndex < Sessions.Num(); SearchIndex++)
	{
		if (Sessions[SearchIndex].SessionName == SessionName)
		{
			Sessions.RemoveAtSwap(SearchIndex);
			return;
		}
	}
}


EOnlineSessionState::Type FOnlineSessionIOS::GetSessionState(FName SessionName) const
{
	FScopeLock ScopeLock(&SessionLock);
	for (int32 SearchIndex = 0; SearchIndex < Sessions.Num(); SearchIndex++)
	{
		if (Sessions[SearchIndex].SessionName == SessionName)
		{
			return Sessions[SearchIndex].SessionState;
		}
	}

	return EOnlineSessionState::NoSession;
}


bool FOnlineSessionIOS::HasPresenceSession()
{
	FScopeLock ScopeLock(&SessionLock);
	for (int32 SearchIndex = 0; SearchIndex < Sessions.Num(); SearchIndex++)
	{
		if (Sessions[SearchIndex].SessionSettings.bUsesPresence)
		{
			return true;
		}
	}
		
	return false;
}


bool FOnlineSessionIOS::CreateSession(int32 HostingPlayerNum, FName SessionName, const FOnlineSessionSettings& NewSessionSettings)
{
	bool bSuccessfullyCreatedSession = false;
	UE_LOG(LogOnline, Display, TEXT("FOnlineSessionIOS::CreateSession"));
	
	// Check for an existing session
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session == NULL)
	{
		Session = AddNamedSession(SessionName, NewSessionSettings);
		Session->SessionState = EOnlineSessionState::Pending;
		UE_LOG(LogOnline, Display, TEXT("Creating new session."));

		// Create the session object
		FGameCenterSessionDelegate* NewGKSession = [FGameCenterSessionDelegate alloc];
		if( NewGKSession != NULL )
		{
			UE_LOG(LogOnline, Display, TEXT("Created session delegate"));
			NSString* SafeSessionName = [NSString stringWithCString:TCHAR_TO_ANSI(*SessionName.ToString())  encoding:NSASCIIStringEncoding];
			GKSessions.Add(SessionName, NewGKSession);
		}
		else
		{
			UE_LOG(LogOnline, Warning, TEXT("Failed to create session delegate"));
		}
		
		bSuccessfullyCreatedSession = true;
	}
	else
	{
		UE_LOG(LogOnline, Display, TEXT("Cannot create session '%s': session already exists."), *SessionName.ToString());
	}
	
	UE_LOG(LogOnline, Display, TEXT("TriggerOnCreateSessionCompleteDelegates: %s, %d"), *SessionName.ToString(), bSuccessfullyCreatedSession);
	TriggerOnCreateSessionCompleteDelegates(SessionName, bSuccessfullyCreatedSession);

	return bSuccessfullyCreatedSession;
}


bool FOnlineSessionIOS::StartSession(FName SessionName)
{
	bool bSuccessfullyStartedSession = false;
	UE_LOG(LogOnline, Display, TEXT("FOnlineSessionIOS::StartSession"));
		
	// Check for an existing session
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session != NULL)
	{
		// Find the linked GK session and start it.
		FGameCenterSessionDelegate* LinkedGKSession = *GKSessions.Find( SessionName );
		NSString* SafeSessionName = [NSString stringWithCString:TCHAR_TO_ANSI(*SessionName.ToString())  encoding:NSASCIIStringEncoding];
		[LinkedGKSession  initSessionWithName:SafeSessionName];

		// Update the session state as we are now running.
		Session->SessionState = EOnlineSessionState::InProgress;
		bSuccessfullyStartedSession = true;
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("Failed to create session delegate"));
	}

	TriggerOnStartSessionCompleteDelegates(SessionName, bSuccessfullyStartedSession);

	return bSuccessfullyStartedSession;
}


bool FOnlineSessionIOS::UpdateSession(FName SessionName, FOnlineSessionSettings& UpdatedSessionSettings, bool bShouldRefreshOnlineData)
{
	bool bSuccessfullyUpdatedSession = false;
	
	UE_LOG(LogOnline, Display, TEXT("FOnlineSessionIOS::UpdateSession - not implemented"));
	
	TriggerOnUpdateSessionCompleteDelegates(SessionName, bSuccessfullyUpdatedSession);
	
	return bSuccessfullyUpdatedSession;
}


bool FOnlineSessionIOS::EndSession(FName SessionName)
{
	bool bSuccessfullyEndedSession = false;
	
	UE_LOG(LogOnline, Display, TEXT("FOnlineSessionIOS::EndSession - not implemented"));

	TriggerOnEndSessionCompleteDelegates(SessionName, bSuccessfullyEndedSession);

	return bSuccessfullyEndedSession;
}


bool FOnlineSessionIOS::DestroySession(FName SessionName)
{
	bool bSuccessfullyDestroyedSession = false;
	
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session == NULL)
	{
		FGameCenterSessionDelegate* ExistingGKSession = *GKSessions.Find( SessionName );
		[ExistingGKSession shutdownSession];
		
		// The session info is no longer needed
		RemoveNamedSession( Session->SessionName );

		GKSessions.Remove( SessionName );

		bSuccessfullyDestroyedSession = true;
	}

	TriggerOnDestroySessionCompleteDelegates(SessionName, bSuccessfullyDestroyedSession);

	return bSuccessfullyDestroyedSession;
}

bool FOnlineSessionIOS::IsPlayerInSession(FName SessionName, const FUniqueNetId& UniqueId)
{
	return IsPlayerInSessionImpl(this, SessionName, UniqueId);
}

bool FOnlineSessionIOS::FindSessions(int32 SearchingPlayerNum, const TSharedRef<FOnlineSessionSearch>& SearchSettings)
{
	bool bSuccessfullyFoundSessions = false;

	// Don't start another search while one is in progress
	if (!CurrentSessionSearch.IsValid() && SearchSettings->SearchState != EOnlineAsyncTaskState::InProgress)
	{
		for (TMap< FName, FGameCenterSessionDelegate* >::TConstIterator SessionIt(GKSessions); SessionIt; ++SessionIt)
		{
			FGameCenterSessionDelegate* GKSession = SessionIt.Value();
			NSArray* availablePeers = [[GKSession Session] peersWithConnectionState:GKPeerStateAvailable];
			bSuccessfullyFoundSessions = [availablePeers count] > 0;
		}
	}

	TriggerOnFindSessionsCompleteDelegates(bSuccessfullyFoundSessions);

	return bSuccessfullyFoundSessions;
}


bool FOnlineSessionIOS::CancelFindSessions()
{
	bool bSuccessfullyCancelledSession = false;

	UE_LOG(LogOnline, Display, TEXT("FOnlineSessionIOS::CancelSession - not implemented"));
	
	TriggerOnCancelFindSessionsCompleteDelegates(true);

	return bSuccessfullyCancelledSession;
}


bool FOnlineSessionIOS::PingSearchResults(const FOnlineSessionSearchResult& SearchResult)
{
	bool bSuccessfullyPingedSearchResults = false;

	UE_LOG(LogOnline, Display, TEXT("FOnlineSessionIOS::PingSearchResults - not implemented"));
	
	return bSuccessfullyPingedSearchResults;
}


bool FOnlineSessionIOS::JoinSession(int32 PlayerNum, FName SessionName, const FOnlineSessionSearchResult& DesiredSession)
{
	bool bSuccessfullyJointSession = false;
	UE_LOG(LogOnline, Display, TEXT("FOnlineSessionIOS::JoinSession"));

	FGameCenterSessionDelegate* SessionDelegate = *GKSessions.Find( SessionName );
	if( SessionDelegate != NULL )
	{
		NSString* PeerID = [[SessionDelegate Session] peerID];
		[SessionDelegate connectToPeer:PeerID];

		bSuccessfullyJointSession = true;
	}

	TriggerOnJoinSessionCompleteDelegates(SessionName, bSuccessfullyJointSession);

	return bSuccessfullyJointSession;
}


bool FOnlineSessionIOS::FindFriendSession(int32 LocalUserNum, const FUniqueNetId& Friend)
{
	bool bSuccessfullyJointFriendSession = false;
	
	UE_LOG(LogOnline, Display, TEXT("FOnlineSessionIOS::FindFriendSession - not implemented"));
	
	FOnlineSessionSearchResult EmptyResult;
	TriggerOnFindFriendSessionCompleteDelegates(LocalUserNum, bSuccessfullyJointFriendSession, EmptyResult);

	return bSuccessfullyJointFriendSession;
}


bool FOnlineSessionIOS::SendSessionInviteToFriend(int32 LocalUserNum, FName SessionName, const FUniqueNetId& Friend)
{
	bool bSuccessfullySentSessionInviteToFriend = false;
	
	UE_LOG(LogOnline, Display, TEXT("FOnlineSessionIOS::SendSessionInviteToFriend - not implemented"));
	
	return bSuccessfullySentSessionInviteToFriend;
}


bool FOnlineSessionIOS::SendSessionInviteToFriends(int32 LocalUserNum, FName SessionName, const TArray< TSharedRef<FUniqueNetId> >& Friends)
{
	bool bSuccessfullySentSessionInviteToFriends = false;

	UE_LOG(LogOnline, Display, TEXT("FOnlineSessionIOS::SendSessionInviteToFriends - not implemented"));
	
	return bSuccessfullySentSessionInviteToFriends;
}


bool FOnlineSessionIOS::GetResolvedConnectString(FName SessionName, FString& ConnectInfo)
{
	bool bSuccessfullyGotResolvedConnectString = false;
	
	UE_LOG(LogOnline, Display, TEXT("FOnlineSessionIOS::GetResolvedConnectString - not implemented"));
	
	return bSuccessfullyGotResolvedConnectString;
}


bool FOnlineSessionIOS::GetResolvedConnectString(const class FOnlineSessionSearchResult& SearchResult, FName PortType, FString& ConnectInfo)
{
	return false;
}


FOnlineSessionSettings* FOnlineSessionIOS::GetSessionSettings(FName SessionName)
{
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		return &Session->SessionSettings;
	}
	return NULL;
}


bool FOnlineSessionIOS::RegisterPlayer(FName SessionName, const FUniqueNetId& PlayerId, bool bWasInvited)
{
	bool bSuccessfullyRegisteredPlayer = false;
	
	UE_LOG(LogOnline, Display, TEXT("FOnlineSessionIOS::RegisterPlayer - not implemented"));	
	
	TArray< TSharedRef<FUniqueNetId> > Players;
	Players.Add(MakeShareable(new FUniqueNetIdString(PlayerId)));
	
	bSuccessfullyRegisteredPlayer = RegisterPlayers(SessionName, Players, bWasInvited);

	return bSuccessfullyRegisteredPlayer;
}


bool FOnlineSessionIOS::RegisterPlayers(FName SessionName, const TArray< TSharedRef<FUniqueNetId> >& Players, bool bWasInvited)
{
	bool bSuccessfullyRegisteredPlayers = false;
	
	UE_LOG(LogOnline, Display, TEXT("FOnlineSessionIOS::RegisterPlayers - not implemented"));
	
	for (int32 PlayerIdx=0; PlayerIdx < Players.Num(); PlayerIdx++)
	{
		TriggerOnRegisterPlayersCompleteDelegates(SessionName, Players, bSuccessfullyRegisteredPlayers);
	}

	return bSuccessfullyRegisteredPlayers;
}


bool FOnlineSessionIOS::UnregisterPlayer(FName SessionName, const FUniqueNetId& PlayerId)
{
	bool bSuccessfullyUnregisteredPlayer = false;
	
	UE_LOG(LogOnline, Display, TEXT("FOnlineSessionIOS::UnregisterPlayer - not implemented"));
	
	TArray< TSharedRef<FUniqueNetId> > Players;
	Players.Add(MakeShareable(new FUniqueNetIdString(PlayerId)));
	bSuccessfullyUnregisteredPlayer = UnregisterPlayers(SessionName, Players);
	
	return bSuccessfullyUnregisteredPlayer;
}


bool FOnlineSessionIOS::UnregisterPlayers(FName SessionName, const TArray< TSharedRef<FUniqueNetId> >& Players)
{
	bool bSuccessfullyUnregisteredPlayers = false;

	UE_LOG(LogOnline, Display, TEXT("FOnlineSessionIOS::UnregisterPlayers - not implemented"));

	for (int32 PlayerIdx=0; PlayerIdx < Players.Num(); PlayerIdx++)
	{
		TriggerOnUnregisterPlayersCompleteDelegates(SessionName, Players, bSuccessfullyUnregisteredPlayers);
	}

	return bSuccessfullyUnregisteredPlayers;
}


int32 FOnlineSessionIOS::GetNumSessions()
{
	FScopeLock ScopeLock(&SessionLock);
	return Sessions.Num();
}


void FOnlineSessionIOS::DumpSessionState()
{

}