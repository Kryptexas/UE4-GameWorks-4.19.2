// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GameState.cpp: GameState C++ code.
=============================================================================*/

#include "EnginePrivate.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogGameState, Log, All);

AGameState::AGameState(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP
		.DoNotCreateDefaultSubobject(TEXT("Sprite")) )
{
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bAlwaysRelevant = true;
	bReplicateMovement = false;

	// Note: this is very important to set to false. Though all replication infos are spawned at run time, during seamless travel
	// they are held on to and brought over into the new world. In ULevel::InitializeActors, these PlayerStates may be treated as map/startup actors
	// and given static NetGUIDs. This also causes their deletions to be recorded and sent to new clients, which if unlucky due to name conflicts,
	// may end up deleting the new PlayerStates they had just spaned.
	bNetLoadOnClient = false;
}

/** Helper to return the default object of the GameMode class corresponding to this GameState. */
AGameMode* AGameState::GetDefaultGameMode() const
{
	if ( GameModeClass )
	{
		AGameMode* const DefaultGameActor = GameModeClass->GetDefaultObject<AGameMode>();
		return DefaultGameActor;
	}
	return NULL;
}

void AGameState::DefaultTimer()
{
	AGameMode* const GameMode = GetWorld()->GetAuthGameMode();

	if ( (GameMode == NULL) || GameMode->bMatchIsInProgress )
	{
		++ElapsedTime;
	}

	GetWorldTimerManager().SetTimer(this, &AGameState::DefaultTimer, GetWorldSettings()->GetEffectiveTimeDilation(), true);
}

void AGameState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	GetWorldTimerManager().SetTimer(this, &AGameState::DefaultTimer, GetWorldSettings()->GetEffectiveTimeDilation(), true);

	UWorld* World = GetWorld();
	World->GameState = this;

	for (FActorIterator It(World); It; ++It)
	{
		AActor* TestActor = *It;
		if (TestActor && !TestActor->IsPendingKill() && TestActor->IsA<APlayerState>())
		{
			APlayerState* PlayerState = Cast<APlayerState>(TestActor);
			AddPlayerState(PlayerState);
		}
	}
}

void AGameState::OnRep_GameModeClass()
{
	ReceivedGameModeClass();
}

void AGameState::OnRep_SpectatorClass()
{
	ReceivedSpectatorClass();
}

void AGameState::OnRep_bMatchHasBegun()
{
	if (bMatchHasBegun)
	{
		GetWorldSettings()->NotifyMatchStarted();
	}
}

void AGameState::OnRep_bMatchIsOver()
{
	if ( bMatchIsOver )
	{
		EndGame();
	}
}

void AGameState::ReceivedGameModeClass()
{
	// Tell each PlayerController that the Game class is here
	for( FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator )
	{
		APlayerController* const PlayerController = *Iterator;
		if (PlayerController)
		{
			PlayerController->ReceivedGameModeClass(GameModeClass);
		}
	}
}

void AGameState::ReceivedSpectatorClass()
{
	// Tell each PlayerController that the Spectator class is here
	for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
	{
		if (It->PlayerController)
		{
			It->PlayerController->ReceivedSpectatorClass(GameModeClass);
		}
	}
}

void AGameState::AddPlayerState(APlayerState* PlayerState)
{
	// Determine whether it should go in the active or inactive list
	if (!PlayerState->bIsInactive)
	{
		// make sure no duplicates
		for (int32 i=0; i<PlayerArray.Num(); i++)
		{
			if (PlayerArray[i] == PlayerState)
				return;
		}

		PlayerArray.Add(PlayerState);
	}
	else
	{
		// Add once only
		if (InactivePlayerArray.Find(PlayerState) == INDEX_NONE)
		{
			InactivePlayerArray.Add(PlayerState);
		}
	}
}

void AGameState::RemovePlayerState(APlayerState* PlayerState)
{
	for (int32 i=0; i<PlayerArray.Num(); i++)
	{
		if (PlayerArray[i] == PlayerState)
		{
			PlayerArray.RemoveAt(i,1);
			return;
		}
	}
}

void AGameState::StartMatch()
{
	bMatchHasBegun = true;
}

void AGameState::EndGame()
{
	bMatchIsOver = true;
}


bool AGameState::ShouldShowGore() const
{
	return true;
}

void AGameState::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( AGameState, bMatchHasBegun );
	DOREPLIFETIME( AGameState, bMatchIsOver );
	DOREPLIFETIME( AGameState, SpectatorClass );

	DOREPLIFETIME_CONDITION( AGameState, GameModeClass,	COND_InitialOnly );
	DOREPLIFETIME_CONDITION( AGameState, ElapsedTime,	COND_InitialOnly );
}
