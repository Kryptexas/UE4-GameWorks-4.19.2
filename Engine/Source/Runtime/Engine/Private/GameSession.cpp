// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GameSession.cpp: GameSession code.
=============================================================================*/

#include "EnginePrivate.h"
#include "Net/UnrealNetwork.h"
#include "Online.h"

DEFINE_LOG_CATEGORY_STATIC(LogGameSession, Log, All);

/** 
 * Returns the player controller associated with this net id
 * @param PlayerNetId the id to search for
 * @return the player controller if found, otherwise NULL
 */
APlayerController* GetPlayerControllerFromNetId(UWorld* World, const FUniqueNetId& PlayerNetId)
{
	// Iterate through the controller list looking for the net id
	for(FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = *Iterator;
		// Determine if this is a player with replication
		if (PlayerController->PlayerState != NULL)
		{
			// If the ids match, then this is the right player.
			if (*PlayerController->PlayerState->UniqueId == PlayerNetId)
			{
				return PlayerController;
			}
		}
	}
	return NULL;
}

AGameSession::AGameSession(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void AGameSession::StartPendingMatch() {}
void AGameSession::EndPendingMatch() {}

bool AGameSession::HandleStartMatch() 
{
	return false;
}

void AGameSession::InitOptions( const FString& Options )
{
	UWorld* World = GetWorld();
	check(World);
	AGameMode* const GameMode = World->GetAuthGameMode();

	MaxPlayers = GameMode->GetIntOption( Options, TEXT("MaxPlayers"), MaxPlayers );
	MaxSpectators = GameMode->GetIntOption( Options, TEXT("MaxSpectators"), MaxSpectators );
	SessionName = GetDefault<APlayerState>(GameMode->PlayerStateClass)->SessionName;
}

bool AGameSession::ProcessAutoLogin()
{
	IOnlineIdentityPtr IdentityInt = Online::GetIdentityInterface();
	if (IdentityInt.IsValid())
	{
		IdentityInt->AddOnLoginChangedDelegate(FOnLoginChangedDelegate::CreateUObject(this, &AGameSession::OnLoginChanged));
		IdentityInt->AddOnLoginCompleteDelegate(0, FOnLoginCompleteDelegate::CreateUObject(this, &AGameSession::OnLoginComplete));
		if (!IdentityInt->AutoLogin(0))
		{
			// Not waiting for async login
			return false;
		}

		return true;
	}

	// Not waiting for async login
	return false;
}

void AGameSession::OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	IOnlineIdentityPtr IdentityInt = Online::GetIdentityInterface();
	if (IdentityInt.IsValid())
	{
		IdentityInt->ClearOnLoginChangedDelegate(FOnLoginChangedDelegate::CreateUObject(this, &AGameSession::OnLoginChanged));
		IdentityInt->ClearOnLoginCompleteDelegate(0, FOnLoginCompleteDelegate::CreateUObject(this, &AGameSession::OnLoginComplete));
	}
}

void AGameSession::OnLoginChanged(int32 LocalUserNum)
{
	IOnlineIdentityPtr IdentityInt = Online::GetIdentityInterface();
	if (IdentityInt.IsValid())
	{
		IdentityInt->ClearOnLoginChangedDelegate(FOnLoginChangedDelegate::CreateUObject(this, &AGameSession::OnLoginChanged));
		IdentityInt->ClearOnLoginCompleteDelegate(0, FOnLoginCompleteDelegate::CreateUObject(this, &AGameSession::OnLoginComplete));
		if (IdentityInt->GetLoginStatus(0) == ELoginStatus::LoggedIn)
		{
			RegisterServer();
		}
	}
}

void AGameSession::RegisterServer()
{
}

FString AGameSession::ApproveLogin(const FString& Options)
{
	UWorld* const World = GetWorld();
	check(World);

	AGameMode* const GameMode = World->GetAuthGameMode();
	check(GameMode);

	int32 SpectatorOnly = 0;
	SpectatorOnly = GameMode->GetIntOption(Options, TEXT("SpectatorOnly"), SpectatorOnly);

	if (AtCapacity(SpectatorOnly == 1))
	{
		return TEXT("Engine.GameMessage.MaxedOutMessage");
	}

	int32 SplitscreenCount = 0;
	SplitscreenCount = GameMode->GetIntOption(Options, TEXT("SplitscreenCount"), SplitscreenCount);

	if (SplitscreenCount > MaxSplitscreensPerConnection)
	{
		return FString::Printf(TEXT("A maximum of '%i' splitscreen players are allowed"), MaxSplitscreensPerConnection);
	}

	return TEXT("");
}

void AGameSession::PostLogin(APlayerController* NewPlayer)
{
}

/** @return A new unique player ID */
int32 AGameSession::GetNextPlayerID()
{
	// Start at 256, because 255 is special (means all team for some UT Emote stuff)
	static int32 NextPlayerID = 256;
	return NextPlayerID++;
}

/**
 * Register a player with the online service session
 * 
 * @param NewPlayer player to register
 * @param UniqueId uniqueId they sent over on Login
 * @param bWasFromInvite was this from an invite
 */
void AGameSession::RegisterPlayer(APlayerController* NewPlayer, const TSharedPtr<FUniqueNetId>& UniqueId, bool bWasFromInvite)
{
	if (NewPlayer != NULL)
	{
		// Set the player's ID.
		check(NewPlayer->PlayerState);
		NewPlayer->PlayerState->PlayerId = GetNextPlayerID();
		NewPlayer->PlayerState->SetUniqueId(UniqueId);
		NewPlayer->PlayerState->RegisterPlayerWithSession(bWasFromInvite);
	}
}

/**
 * Unregister a player from the online service session
 */
void AGameSession::UnregisterPlayer(APlayerController* ExitingPlayer)
{
	IOnlineSessionPtr SessionInt = Online::GetSessionInterface();
	if (SessionInt.IsValid())
	{
		if (GetNetMode() != NM_Standalone && 
			ExitingPlayer != NULL &&
			ExitingPlayer->PlayerState && 
			ExitingPlayer->PlayerState->UniqueId.IsValid() &&
			ExitingPlayer->PlayerState->UniqueId->IsValid())
		{
			// Remove the player from the session
			SessionInt->UnregisterPlayer(ExitingPlayer->PlayerState->SessionName, *ExitingPlayer->PlayerState->UniqueId);
		}
	}
}

bool AGameSession::AtCapacity(bool bSpectator)
{
	if ( GetNetMode() == NM_Standalone )
	{
		return false;
	}

	if ( bSpectator )
	{
		return ( (GetWorld()->GetAuthGameMode()->NumSpectators >= MaxSpectators)
		&& ((GetNetMode() != NM_ListenServer) || (GetWorld()->GetAuthGameMode()->NumPlayers > 0)) );
	}
	else
	{
		return ( (MaxPlayers>0) && (GetWorld()->GetAuthGameMode()->GetNumPlayers() >= MaxPlayers) );
	}
}

void AGameSession::NotifyLogout(APlayerController* PC)
{
	// Unregister the player from the online layer
	UnregisterPlayer(PC);
}

bool AGameSession::KickPlayer(APlayerController* C, const FString& KickReason)
{
	// Do not kick logged admins
	if (C != NULL && Cast<UNetConnection>(C->Player)!=NULL )
	{
		if (C->GetPawn() != NULL)
		{
			C->GetPawn()->Destroy();
		}
		C->ClientWasKicked();
		if (C != NULL)
		{
			C->Destroy();
		}
		return true;
	}
	return false;
}

void AGameSession::ReturnToMainMenuHost()
{
	FString RemoteReturnReason = NSLOCTEXT("NetworkErrors", "HostHasLeft", "Host has left the game.").ToString();
	FString LocalReturnReason(TEXT(""));

	APlayerController* Controller = NULL;
	FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator();
	for(; Iterator; ++Iterator)
	{
		Controller = *Iterator;
		if (Controller && !Controller->IsLocalPlayerController() && Controller->IsPrimaryPlayer())
		{
			// Clients
			Controller->ClientReturnToMainMenu(RemoteReturnReason);
		}
	}

	Iterator.Reset();
	for(; Iterator; ++Iterator)
	{
		Controller = *Iterator;
		if (Controller && Controller->IsLocalPlayerController() && Controller->IsPrimaryPlayer())
		{
			Controller->ClientReturnToMainMenu(LocalReturnReason);
			break;
		}
	}
}

void AGameSession::PostSeamlessTravel()
{
}

void AGameSession::DumpSessionState()
{
	UE_LOG(LogGameSession, Log, TEXT("  MaxPlayers: %i"), MaxPlayers);
	UE_LOG(LogGameSession, Log, TEXT("  MaxSpectators: %i"), MaxSpectators);

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->DumpSessionState();
		}
	}
}

bool AGameSession::CanRestartGame()
{
	return true;
}

void AGameSession::EndGame()
{
}

void AGameSession::UpdateSessionJoinability(FName InSessionName, bool bPublicSearchable, bool bAllowInvites, bool bJoinViaPresence, bool bJoinViaPresenceFriendsOnly)
{
	if (GetNetMode() != NM_Standalone)
	{
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
		if (OnlineSub)
		{
			IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
			if (Sessions.IsValid())
			{
				FOnlineSessionSettings* GameSettings = Sessions->GetSessionSettings(InSessionName);
				if (GameSettings != NULL)
				{
					GameSettings->bAllowInvites = bAllowInvites;
					GameSettings->bAllowJoinInProgress = bPublicSearchable;
					GameSettings->bAllowJoinViaPresence = bJoinViaPresence && !bJoinViaPresenceFriendsOnly;
					GameSettings->bAllowJoinViaPresenceFriendsOnly = bJoinViaPresenceFriendsOnly;
					Sessions->UpdateSession(InSessionName, *GameSettings, true);
				}
			}
		}
	}
}
