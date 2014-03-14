// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemUtilsPrivatePCH.h"
#include "TestSessionInterface.h"
#include "OnlineIdentityInterface.h"
#include "OnlineFriendsInterface.h"
#include "OnlineExternalUIInterface.h"
#include "ModuleManager.h"

#include "Engine.h"
#include "EngineKismetLibraryClasses.h"

/**
 *	Example of a hosted session
 */
class TestOnlineGameSettings : public FOnlineSessionSettings
{
 public:
 	TestOnlineGameSettings(bool bTestingLAN = false, bool bTestingPresence = false)
	{
 		NumPublicConnections = 10;
		NumPrivateConnections = 0;
		bIsLANMatch = bTestingLAN;
		bShouldAdvertise = true;
		bAllowJoinInProgress = true;
		bAllowInvites = true;
		bUsesPresence = bTestingPresence;
		bAllowJoinViaPresence = true;
		bAllowJoinViaPresenceFriendsOnly = false;

		Set(FName(TEXT("TESTSETTING1")), (int32)5, EOnlineDataAdvertisementType::ViaOnlineService);
		Set(FName(TEXT("TESTSETTING2")), (float)5.0f, EOnlineDataAdvertisementType::ViaOnlineService);
		Set(FName(TEXT("TESTSETTING3")), FString(TEXT("Hello")), EOnlineDataAdvertisementType::ViaOnlineService);
		Set(FName(TEXT("TESTSETTING4")), FString(TEXT("Test4")), EOnlineDataAdvertisementType::ViaPingOnly);
		Set(FName(TEXT("TESTSETTING5")), FString(TEXT("Test5")), EOnlineDataAdvertisementType::ViaPingOnly);

 	}
 	
 	virtual ~TestOnlineGameSettings()
 	{
 
 	}

	void AddWorldSettings(UWorld* InWorld)
	{
		if (InWorld)
		{
			const FString MapName = InWorld->GetMapName();
			Set(SETTING_MAPNAME, MapName, EOnlineDataAdvertisementType::ViaOnlineService);

			AGameMode const* const GameMode = InWorld->GetAuthGameMode();
			if (GameMode != NULL)
			{
				// Bot count
				Set(SETTING_NUMBOTS, GameMode->NumBots, EOnlineDataAdvertisementType::ViaOnlineService);

				// Game type
				FString GameModeStr = GameMode->GetClass()->GetName();
				Set(SETTING_GAMEMODE, GameModeStr, EOnlineDataAdvertisementType::ViaOnlineService);
			}
		}
	}
 };

/**
 *	Example of a session search query
 */
class TestOnlineSearchSettings : public FOnlineSessionSearch
{
public:
	TestOnlineSearchSettings(bool bSearchingLAN = false, bool bSearchingPresence = false)
	{
		bIsLanQuery = bSearchingLAN;
		MaxSearchResults = 10;
		PingBucketSize = 50;

		QuerySettings.Set(FName(TEXT("TESTSETTING1")), (int32)5, EOnlineComparisonOp::Equals);
		QuerySettings.Set(FName(TEXT("TESTSETTING2")), (float)5.0f, EOnlineComparisonOp::Equals);
		QuerySettings.Set(FName(TEXT("TESTSETTING3")), FString(TEXT("Hello")), EOnlineComparisonOp::Equals);

		if (bSearchingPresence)
		{
			QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
		}
	}

	virtual ~TestOnlineSearchSettings()
	{

	}
};

void FTestSessionInterface::Test(UWorld* InWorld, bool bTestLAN, bool bIsPresence)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get(FName(*Subsystem));
	check(OnlineSub); 

	World = InWorld;
	GEngine->OnWorldDestroyed().AddRaw(this, &FTestSessionInterface::WorldDestroyed);

	if (OnlineSub->GetIdentityInterface().IsValid())
	{
		UserId = OnlineSub->GetIdentityInterface()->GetUniquePlayerId(0);
	}
	
	// Cache interfaces
	Identity = OnlineSub->GetIdentityInterface();
	Sessions = OnlineSub->GetSessionInterface();
	check(Sessions.IsValid());
	Friends = OnlineSub->GetFriendsInterface();

	// Define delegates
	OnReadFriendsListCompleteDelegate = FOnReadFriendsListCompleteDelegate::CreateRaw(this, &FTestSessionInterface::OnReadFriendsListComplete);

	OnCreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateRaw(this, &FTestSessionInterface::OnCreateSessionComplete);
	OnStartSessionCompleteDelegate = FOnStartSessionCompleteDelegate::CreateRaw(this, &FTestSessionInterface::OnStartSessionComplete);
	OnEndSessionCompleteDelegate = FOnEndSessionCompleteDelegate::CreateRaw(this, &FTestSessionInterface::OnEndSessionComplete);
	OnDestroySessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateRaw(this, &FTestSessionInterface::OnDestroySessionComplete);

	OnUpdateSessionCompleteDelegate = FOnUpdateSessionCompleteDelegate::CreateRaw(this, &FTestSessionInterface::OnUpdateSessionComplete);

	OnJoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateRaw(this, &FTestSessionInterface::OnJoinSessionComplete);
	OnEndForJoinSessionCompleteDelegate = FOnEndSessionCompleteDelegate::CreateRaw(this, &FTestSessionInterface::OnEndForJoinSessionComplete);
	OnDestroyForJoinSessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateRaw(this, &FTestSessionInterface::OnDestroyForJoinSessionComplete);

	OnFindFriendSessionCompleteDelegate = FOnFindFriendSessionCompleteDelegate::CreateRaw(this, &FTestSessionInterface::OnFindFriendSessionComplete);

	OnRegisterPlayersCompleteDelegate = FOnRegisterPlayersCompleteDelegate::CreateRaw(this, &FTestSessionInterface::OnRegisterPlayerComplete);
	OnUnregisterPlayersCompleteDelegate = FOnRegisterPlayersCompleteDelegate::CreateRaw(this, &FTestSessionInterface::OnUnregisterPlayerComplete);

	OnFindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateRaw(this, &FTestSessionInterface::OnFindSessionsComplete);
	OnCancelFindSessionsCompleteDelegate = FOnCancelFindSessionsCompleteDelegate::CreateRaw(this, &FTestSessionInterface::OnCancelFindSessionsComplete);

	// Read friends list and cache it
	if (Friends.IsValid())
	{
		Friends->AddOnReadFriendsListCompleteDelegate(0, OnReadFriendsListCompleteDelegate);
		Friends->ReadFriendsList(0, EFriendsLists::ToString(EFriendsLists::Default));
	}

	// Setup sessions
	if (bIsHost)
	{
 		HostSettings = MakeShareable(new TestOnlineGameSettings(bTestLAN, bIsPresence));
		HostSettings->AddWorldSettings(InWorld);
 		Sessions->AddOnCreateSessionCompleteDelegate(OnCreateSessionCompleteDelegate);
 		Sessions->CreateSession(0, TEXT("Game"), *HostSettings);
	}
	else
	{
		SearchSettings = MakeShareable(new TestOnlineSearchSettings(bTestLAN, bIsPresence));
		TSharedRef<FOnlineSessionSearch> SearchSettingsRef = SearchSettings.ToSharedRef();

		Sessions->AddOnFindSessionsCompleteDelegate(OnFindSessionsCompleteDelegate);
		Sessions->FindSessions(0, SearchSettingsRef);
	}

	// Always on the lookout for invite acceptance (via actual invite or join in an external client)
	OnSessionInviteAcceptedDelegate = FOnSessionInviteAcceptedDelegate::CreateRaw(this, &FTestSessionInterface::OnSessionInviteAccepted);
	Sessions->AddOnSessionInviteAcceptedDelegate(0, OnSessionInviteAcceptedDelegate);
}

void FTestSessionInterface::ClearDelegates()
{
	Sessions->ClearOnSessionInviteAcceptedDelegate(0, OnSessionInviteAcceptedDelegate);
	GEngine->OnWorldDestroyed().RemoveAll(this);
}

void FTestSessionInterface::WorldDestroyed( UWorld* InWorld )
{
	if (InWorld == World)
	{
		World = NULL;
	}
}

void FTestSessionInterface::OnReadFriendsListComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr)
{
	UE_LOG(LogOnline, Verbose, TEXT("OnReadFriendsListComplete LocalUserNum: %d bSuccess: %d. "), LocalUserNum, bWasSuccessful, *ErrorStr);
	Friends->ClearOnReadFriendsListCompleteDelegate(0, OnReadFriendsListCompleteDelegate);
	if (bWasSuccessful)
	{
		Friends->GetFriendsList(LocalUserNum, ListName, FriendsCache);
	}
}

void FTestSessionInterface::OnSessionInviteAccepted(int32 LocalUserNum, bool bWasSuccessful, const FOnlineSessionSearchResult& SearchResult)
{
	UE_LOG(LogOnline, Verbose, TEXT("OnSessionInviteAccepted LocalUserNum: %d bSuccess: %d"), LocalUserNum, bWasSuccessful);
	// Don't clear invite accept delegate

	if (bWasSuccessful)
	{
		JoinSession(LocalUserNum, TEXT("Game"), SearchResult);
	}
}

void FTestSessionInterface::OnEndForJoinSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogOnline, Verbose, TEXT("OnEndForJoinSessionComplete %s bSuccess: %d"), *SessionName.ToString(), bWasSuccessful);
	Sessions->ClearOnEndSessionCompleteDelegate(OnEndForJoinSessionCompleteDelegate);
	DestroyExistingSession(SessionName, OnDestroyForJoinSessionCompleteDelegate);
}

void FTestSessionInterface::EndExistingSession(FName SessionName, FOnEndSessionCompleteDelegate& Delegate)
{
	Sessions->AddOnEndSessionCompleteDelegate(Delegate);
	Sessions->EndSession(SessionName);
}

void FTestSessionInterface::OnDestroyForJoinSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogOnline, Verbose, TEXT("OnDestroyForJoinSessionComplete %s bSuccess: %d"), *SessionName.ToString(), bWasSuccessful);
	Sessions->ClearOnDestroySessionCompleteDelegate(OnDestroyForJoinSessionCompleteDelegate);
	JoinSession(0, SessionName, CachedSessionResult);
}

void FTestSessionInterface::DestroyExistingSession(FName SessionName, FOnDestroySessionCompleteDelegate& Delegate)
{
	Sessions->AddOnDestroySessionCompleteDelegate(Delegate);
	Sessions->DestroySession(SessionName);
}

void FTestSessionInterface::OnRegisterPlayerComplete(FName SessionName, const TArray< TSharedRef<FUniqueNetId> >& Players, bool bWasSuccessful)
{
	UE_LOG(LogOnline, Verbose, TEXT("OnRegisterPlayerComplete %s bSuccess: %d"), *SessionName.ToString(), bWasSuccessful);
	Sessions->ClearOnRegisterPlayersCompleteDelegate(OnRegisterPlayersCompleteDelegate);
}

void FTestSessionInterface::OnUnregisterPlayerComplete(FName SessionName, const TArray< TSharedRef<FUniqueNetId> >& Players, bool bWasSuccessful)
{
	UE_LOG(LogOnline, Verbose, TEXT("OnUnregisterPlayerComplete %s bSuccess: %d"), *SessionName.ToString(), bWasSuccessful);
	Sessions->ClearOnUnregisterPlayersCompleteDelegate(OnUnregisterPlayersCompleteDelegate);
}

void FTestSessionInterface::JoinSession(int32 LocalUserNum, FName SessionName, const FOnlineSessionSearchResult& SearchResult)
{
	// Clean up existing sessions if applicable
	EOnlineSessionState::Type SessionState = Sessions->GetSessionState(SessionName);
	if (SessionState != EOnlineSessionState::NoSession)
	{
		CachedSessionResult = SearchResult;
		EndExistingSession(SessionName, OnEndForJoinSessionCompleteDelegate);
	}
	else
	{
		Sessions->AddOnJoinSessionCompleteDelegate(OnJoinSessionCompleteDelegate);
		Sessions->JoinSession(LocalUserNum, SessionName, SearchResult);
	}
}

void FTestSessionInterface::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogOnline, Verbose, TEXT("OnCreateSessionComplete %s bSuccess: %d"), *SessionName.ToString(), bWasSuccessful);
	Sessions->ClearOnCreateSessionCompleteDelegate(OnCreateSessionCompleteDelegate);

	bWasLastCommandSuccessful = bWasSuccessful;
}

void FTestSessionInterface::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogOnline, Verbose, TEXT("OnStartSessionComplete %s bSuccess: %d"), *SessionName.ToString(), bWasSuccessful);
	Sessions->ClearOnStartSessionCompleteDelegate(OnStartSessionCompleteDelegate);

	bWasLastCommandSuccessful = bWasSuccessful;
}

void FTestSessionInterface::OnEndSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogOnline, Verbose, TEXT("OnEndSessionComplete %s bSuccess: %d"), *SessionName.ToString(), bWasSuccessful);
	Sessions->ClearOnEndSessionCompleteDelegate(OnEndSessionCompleteDelegate);

	bWasLastCommandSuccessful = bWasSuccessful;
}

void FTestSessionInterface::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogOnline, Verbose, TEXT("OnDestroySessionComplete %s bSuccess: %d"), *SessionName.ToString(), bWasSuccessful);
	Sessions->ClearOnDestroySessionCompleteDelegate(OnDestroySessionCompleteDelegate);
}

void FTestSessionInterface::OnUpdateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogOnline, Verbose, TEXT("OnUpdateSessionComplete %s bSuccess: %d"), *SessionName.ToString(), bWasSuccessful);
	Sessions->ClearOnUpdateSessionCompleteDelegate(OnUpdateSessionCompleteDelegate);
}

void FTestSessionInterface::OnJoinSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogOnline, Verbose, TEXT("OnJoinSessionComplete %s bSuccess: %d"), *SessionName.ToString(), bWasSuccessful);
	Sessions->ClearOnJoinSessionCompleteDelegate(OnJoinSessionCompleteDelegate);

	if (bWasSuccessful)
	{
		FString URL;
		if (World && Sessions->GetResolvedConnectString(SessionName, URL))
		{
			APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
			if (PC)
			{
				PC->ClientTravel(URL, TRAVEL_Absolute);
			}
		}
		else
		{
			UE_LOG(LogOnline, Warning, TEXT("Failed to join session %s"), *SessionName.ToString());
		}
	}
}

void FTestSessionInterface::OnFindFriendSessionComplete(int32 LocalUserNum, bool bWasSuccessful, const FOnlineSessionSearchResult& SearchResult)
{
	UE_LOG(LogOnline, Verbose, TEXT("OnFindFriendSessionComplete LocalUserNum: %d bSuccess: %d"), LocalUserNum, bWasSuccessful);
	Sessions->ClearOnFindFriendSessionCompleteDelegate(LocalUserNum, OnFindFriendSessionCompleteDelegate);
	if (bWasSuccessful)
	{
		if (SearchResult.IsValid())
		{
			JoinSession(LocalUserNum, TEXT("Game"), SearchResult);
		}
		else
		{
			UE_LOG(LogOnline, Warning, TEXT("Join friend returned no search result."));
		}
	}
}

void FTestSessionInterface::OnFindSessionsComplete(bool bWasSuccessful)
{
	UE_LOG(LogOnline, Verbose, TEXT("OnFindSessionsComplete bSuccess: %d"), bWasSuccessful);
	Sessions->ClearOnFindSessionsCompleteDelegate(OnFindSessionsCompleteDelegate);

	UE_LOG(LogOnline, Verbose, TEXT("Num Search Results: %d"), SearchSettings->SearchResults.Num());
	for (int32 SearchIdx=0; SearchIdx<SearchSettings->SearchResults.Num(); SearchIdx++)
	{
		const FOnlineSessionSearchResult& SearchResult = SearchSettings->SearchResults[SearchIdx];
		DumpSession(&SearchResult.Session);
	}
}

void FTestSessionInterface::OnCancelFindSessionsComplete(bool bWasSuccessful)
{
	UE_LOG(LogOnline, Verbose, TEXT("OnCancelFindSessionsComplete bSuccess: %d"), bWasSuccessful);
	Sessions->ClearOnCancelFindSessionsCompleteDelegate(OnCancelFindSessionsCompleteDelegate);
}

bool FTestSessionInterface::Tick(float DeltaTime)
{
	if (TestPhase != LastTestPhase)
	{
		LastTestPhase = TestPhase;
		if (!bOverallSuccess)
		{
			UE_LOG(LogOnline, Log, TEXT("Testing failed in phase %d"), LastTestPhase);
			TestPhase = 3;
		}

		switch(TestPhase)
		{
		case 0:
			break;
		case 1:
			UE_LOG(LogOnline, Log, TEXT("TESTING COMPLETE Success:%s!"), bOverallSuccess ? TEXT("true") : TEXT("false"));
			delete this;
			return false;
		}
	}
	return true;
}

bool FTestSessionInterface::Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar )
{
	bool bWasHandled = false;

	int32 LocalUserNum = 0;

	// Ignore any execs that don't start with TESTSESSION
	if (FParse::Command(&Cmd, TEXT("TESTSESSION")))
	{
		// Get optional session name "GAME" otherwise
		FString Token;
		FName SessionName = FParse::Value(Cmd, TEXT("Name="), Token) ? FName(*Token) : TEXT("GAME");
		if (Token.Len() > 0)
		{
			Cmd += FCString::Strlen(TEXT("Name=")) + Token.Len();
		}
		
		if (FParse::Command(&Cmd, TEXT("SEARCH")))
		{
			if (SearchSettings.IsValid())
			{
				TSharedRef<FOnlineSessionSearch> SearchSettingsRef = SearchSettings.ToSharedRef();

				Sessions->AddOnFindSessionsCompleteDelegate(OnFindSessionsCompleteDelegate);
				Sessions->FindSessions(LocalUserNum, SearchSettingsRef);
			}
			bWasHandled = true;
		}
		else if (FParse::Command(&Cmd, TEXT("JOIN")))
		{
			TCHAR SearchIdxStr[256];
			if (FParse::Token(Cmd, SearchIdxStr, ARRAY_COUNT(SearchIdxStr), true))
			{
				int32 SearchIdx = FCString::Atoi(SearchIdxStr);
				if (SearchIdx >= 0 && SearchIdx < SearchSettings->SearchResults.Num())
				{
					JoinSession(LocalUserNum, SessionName, SearchSettings->SearchResults[SearchIdx]);
				}
			}
			bWasHandled = true;
		}
		else if (FParse::Command(&Cmd, TEXT("JOINFRIEND")))
		{
			if (FParse::Command(&Cmd, TEXT("LOBBY")))
			{
				TCHAR FriendNameStr[256];
				if (FParse::Token(Cmd, FriendNameStr, ARRAY_COUNT(FriendNameStr), true))
				{
					for (int32 FriendIdx=0; FriendIdx<FriendsCache.Num(); FriendIdx++)
					{
						const FOnlineFriend& Friend = *FriendsCache[FriendIdx];
						if (Friend.GetDisplayName() == FriendNameStr)
						{
							Sessions->AddOnFindFriendSessionCompleteDelegate(LocalUserNum, OnFindFriendSessionCompleteDelegate);
							Sessions->FindFriendSession(LocalUserNum, *Friend.GetUserId());
							break;
						}
					}
				}
			}
			else
			{
				TCHAR FriendIdStr[256];
				if (FParse::Token(Cmd, FriendIdStr, ARRAY_COUNT(FriendIdStr), true))
				{
					TSharedPtr<FUniqueNetId> FriendId = Identity->CreateUniquePlayerId((uint8*)FriendIdStr, FCString::Strlen(FriendIdStr));
					Sessions->AddOnFindFriendSessionCompleteDelegate(LocalUserNum, OnFindFriendSessionCompleteDelegate);
					Sessions->FindFriendSession(LocalUserNum, *FriendId);
				}
			}
			bWasHandled = true;
		}
		else if (FParse::Command(&Cmd, TEXT("CREATE")))
		{
			// Mutually exclusive settings
			bool bTestLAN = FParse::Command(&Cmd, TEXT("LAN")) ? true : false;
			bool bTestPresence = FParse::Command(&Cmd, TEXT("PRESENCE")) ? true : false;

			HostSettings = MakeShareable(new TestOnlineGameSettings(bTestLAN));
			HostSettings->bUsesPresence = bTestPresence;
			HostSettings->AddWorldSettings(InWorld);

			Sessions->AddOnCreateSessionCompleteDelegate(OnCreateSessionCompleteDelegate);
			Sessions->CreateSession(0, SessionName, *HostSettings);
			bWasHandled = true;
		}
		else if (FParse::Command(&Cmd, TEXT("START")))
		{
			Sessions->AddOnStartSessionCompleteDelegate(OnStartSessionCompleteDelegate);
			Sessions->StartSession(SessionName);
			bWasHandled = true;
		}
		else if (FParse::Command(&Cmd, TEXT("UPDATE")))
		{
			bool bUpdateOnline = FParse::Command(&Cmd, TEXT("ONLINE")) ? true : false;
			Sessions->AddOnUpdateSessionCompleteDelegate(OnUpdateSessionCompleteDelegate);
			HostSettings->Set(FName(TEXT("UPDATESETTING1")), FString(TEXT("Test1")), EOnlineDataAdvertisementType::ViaOnlineService);
			HostSettings->Set(FName(TEXT("UPDATESETTING2")), 99, EOnlineDataAdvertisementType::ViaOnlineService);
			Sessions->UpdateSession(SessionName, *HostSettings, bUpdateOnline);
			bWasHandled = true;
		}
		else if (FParse::Command(&Cmd, TEXT("END")))
		{
			EndExistingSession(SessionName, OnEndSessionCompleteDelegate);
			bWasHandled = true;
		}
		else if (FParse::Command(&Cmd, TEXT("DESTROY")))
		{
			DestroyExistingSession(SessionName, OnDestroySessionCompleteDelegate);
			bWasHandled = true;
		}
		else if (FParse::Command(&Cmd, TEXT("REGISTER")))
		{
			bool bWasInvited = FParse::Command(&Cmd, TEXT("INVITED")) ? true : false;
			Sessions->AddOnRegisterPlayersCompleteDelegate(OnRegisterPlayersCompleteDelegate);
			Sessions->RegisterPlayer(SessionName, *UserId, bWasInvited);
			bWasHandled = true;
		}
		else if (FParse::Command(&Cmd, TEXT("UNREGISTER")))
		{
			Sessions->AddOnUnregisterPlayersCompleteDelegate(OnUnregisterPlayersCompleteDelegate);
			Sessions->UnregisterPlayer(SessionName, *UserId);
			bWasHandled = true;
		}
		else if (FParse::Command(&Cmd, TEXT("INVITE")))
		{
			if (FParse::Command(&Cmd, TEXT("UI")))
			{
				IOnlineSubsystem::Get(FName(*Subsystem))->GetExternalUIInterface()->ShowInviteUI(LocalUserNum);
			}
			else
			{
				TCHAR FriendNameStr[256];
				if (FParse::Token(Cmd, FriendNameStr, ARRAY_COUNT(FriendNameStr), true))
				{
					for (int32 FriendIdx=0; FriendIdx<FriendsCache.Num(); FriendIdx++)
					{
						const FOnlineFriend& Friend = *FriendsCache[FriendIdx];
						if (Friend.GetDisplayName() == FriendNameStr)
						{
							Sessions->SendSessionInviteToFriend(LocalUserNum, SessionName, *Friend.GetUserId());
							break;
						}
					}
				}
			}

			bWasHandled = true;
		}
		else if (FParse::Command(&Cmd, TEXT("DUMPSESSIONS")))
		{
			Sessions->DumpSessionState();
			bWasHandled = true;
		}
		else if (FParse::Command(&Cmd, TEXT("QUIT")))
		{
			UE_LOG(LogOnline, Display, TEXT("Destroying TestSession."));
			Sessions->CancelFindSessions();
			bWasHandled = true;
			// Make this last
			delete this;
		}
	}

	return bWasHandled;
}
