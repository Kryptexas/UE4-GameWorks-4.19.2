// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PlayerState.cpp: 
=============================================================================*/

#include "EnginePrivate.h"
#include "Net/UnrealNetwork.h"
#include "Online.h"
#include "OnlineSubsystemTypes.h"

APlayerState::APlayerState(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP
		.DoNotCreateDefaultSubobject(TEXT("Sprite")) )
{
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bAlwaysRelevant = true;
	bReplicateMovement = false;
	NetUpdateFrequency = 1;

	// Note: this is very important to set to false. Though all replication infos are spawned at run time, during seamless travel
	// they are held on to and brought over into the new world. In ULevel::InitializeActors, these PlayerStates may be treated as map/startup actors
	// and given static NetGUIDs. This also causes their deletions to be recorded and sent to new clients, which if unlucky due to name conflicts,
	// may end up deleting the new PlayerStates they had just spaned.
	bNetLoadOnClient = false;

	EngineMessageClass = UEngineMessage::StaticClass();
	SessionName = TEXT("Game");
}

void APlayerState::UpdatePing(float InPing)
{
	// Limit the size of the ping, to avoid overflowing PingBucket values
	InPing = FMath::Min(1.1f, InPing);

	float CurTime = GetWorld()->RealTimeSeconds;

	if ((CurTime - CurPingBucketTimestamp) >= 1.f)
	{
		// Trigger ping recalculation now, while all buckets are 'full'
		//	(misses the latest ping update, but averages a full 4 seconds data)
		RecalculateAvgPing();

		CurPingBucket = (CurPingBucket + 1) % ARRAY_COUNT(PingBucket);
		CurPingBucketTimestamp = CurTime;


		PingBucket[CurPingBucket].PingSum = FMath::Floor(InPing * 1000.f);
		PingBucket[CurPingBucket].PingCount = 1;
	}
	// Limit the number of pings we accept per-bucket, to avoid overflowing PingBucket values
	else if (PingBucket[CurPingBucket].PingCount < 7)
	{
		PingBucket[CurPingBucket].PingSum += FMath::Floor(InPing * 1000.f);
		PingBucket[CurPingBucket].PingCount++;
	}
}

void APlayerState::RecalculateAvgPing()
{
	int32 Sum = 0;
	int32 Count = 0;

	for (uint8 i=0; i<ARRAY_COUNT(PingBucket); i++)
	{
		Sum += PingBucket[i].PingSum;
		Count += PingBucket[i].PingCount;
	}

	// Calculate the average, and divide it by 4 to optimize replication
	ExactPing = (float)Sum / (float)Count;
	Ping = FMath::Min(255, (int32)(ExactPing * 0.25f));
}


void APlayerState::OverrideWith(APlayerState* PlayerState)
{
	bIsSpectator = PlayerState->bIsSpectator;
	bOnlySpectator = PlayerState->bOnlySpectator;
	SetUniqueId(PlayerState->UniqueId.GetUniqueNetId());
}


void APlayerState::CopyProperties(APlayerState* PlayerState)
{
	PlayerState->Score = Score;
	PlayerState->Ping = Ping;
	PlayerState->PlayerName = PlayerName;
	PlayerState->PlayerId = PlayerId;
	PlayerState->SetUniqueId(UniqueId.GetUniqueNetId());
	PlayerState->StartTime = StartTime;
	PlayerState->SavedNetworkAddress = SavedNetworkAddress;
}

void APlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	UWorld* World = GetWorld();
	// register this PlayerState with the game's ReplicationInfo
	if ( World->GameState != NULL )
	{
		World->GameState->AddPlayerState(this);
	}

	if ( Role < ROLE_Authority )
	{
		return;
	}

	if (Cast<AAIController>(GetOwner()) != NULL)
	{
		bIsABot = true;
	}

	if (World->GameState)
	{
		StartTime = World->GameState->ElapsedTime;
	}
}

void APlayerState::ClientInitialize(AController* C)
{
	SetOwner(C);
}

void APlayerState::OnRep_Score()
{
}

void APlayerState::OnRep_PlayerName()
{
	OldName = PlayerName;

	if ( GetWorld()->TimeSeconds < 2 )
	{
		bHasBeenWelcomed = true;
		return;
	}

	// new player or name change
	if ( bHasBeenWelcomed )
	{
		if( ShouldBroadCastWelcomeMessage() )
		{
			for( FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator )
			{
				APlayerController* PlayerController = *Iterator;
				if( PlayerController )
				{
					PlayerController->ClientReceiveLocalizedMessage( EngineMessageClass, 2, this );
				}
			}
		}
	}
	else
	{
		int32 WelcomeMessageNum = bOnlySpectator ? 16 : 1;
		bHasBeenWelcomed = true;

		if( ShouldBroadCastWelcomeMessage() )
		{
			for( FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator )
			{
				APlayerController* PlayerController = *Iterator;
				if( PlayerController )
				{
					PlayerController->ClientReceiveLocalizedMessage( EngineMessageClass, WelcomeMessageNum, this );
				}
			}
		}
	}
}

void APlayerState::OnRep_bIsInactive()
{
	// remove and re-add from the GameState so it's in the right list
	GetWorld()->GameState->RemovePlayerState(this);
	GetWorld()->GameState->AddPlayerState(this);
}


bool APlayerState::ShouldBroadCastWelcomeMessage(bool bExiting)
{
	return (!bIsInactive && GetNetMode() != NM_Standalone);
}

void APlayerState::Destroyed()
{
	if ( GetWorld()->GameState != NULL )
	{
		GetWorld()->GameState->RemovePlayerState(this);
	}

	if( ShouldBroadCastWelcomeMessage(true) )
	{
		for( FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator )
		{
			APlayerController* PlayerController = *Iterator;
			if( PlayerController )
			{
				PlayerController->ClientReceiveLocalizedMessage( EngineMessageClass, 4, this);
			}
		}
	}

	// Remove the player from the online session
	UnregisterPlayerWithSession();
	Super::Destroyed();
}


void APlayerState::Reset()
{
	Super::Reset();
	Score = 0;
	ForceNetUpdate();
}

FString APlayerState::GetHumanReadableName() const
{
	return PlayerName;
}

void APlayerState::SetPlayerName(const FString& S)
{
	PlayerName = S;

	// RepNotify callback won't get called by net code if we are the server
	ENetMode NetMode = GetNetMode();
	if (NetMode == NM_Standalone || NetMode == NM_ListenServer)
	{
		OnRep_PlayerName();
	}
	OldName = PlayerName;
	ForceNetUpdate();
}

/** 
 * Replication of UniqueId from server
 */
void APlayerState::OnRep_UniqueId()
{
	// Register player with session
	RegisterPlayerWithSession(false);
}

/**
 * Associate a UniqueId with this player
 * 
 * @param InUniqueId unique id to associate with this player
 */
void APlayerState::SetUniqueId(const TSharedPtr<FUniqueNetId>& InUniqueId)
{
	UniqueId.SetUniqueNetId(InUniqueId);
}

/** 
 * Register a player with the online subsystem
 *
 * @param bWasFromInvite was this player invited directly
 */
void APlayerState::RegisterPlayerWithSession(bool bWasFromInvite)
{
	if (GetNetMode() != NM_Standalone)
	{
		IOnlineSessionPtr SessionInt = Online::GetSessionInterface();
		if (SessionInt.IsValid() && UniqueId.IsValid())
		{
			// Register the player as part of the session
			const APlayerState* PlayerState = GetDefault<APlayerState>();
			SessionInt->RegisterPlayer(PlayerState->SessionName, *UniqueId, bWasFromInvite);
		}
	}
}

/** 
 * Unregister a player with the online subsystem
 */
void APlayerState::UnregisterPlayerWithSession()
{
	if (GetNetMode() == NM_Client && UniqueId.IsValid())
	{
		const APlayerState* PlayerState = GetDefault<APlayerState>();
		if (PlayerState->SessionName != NAME_None)
		{
			IOnlineSessionPtr SessionInt = Online::GetSessionInterface();
			if (SessionInt.IsValid())
			{
				// Remove the player from the session
				SessionInt->UnregisterPlayer(PlayerState->SessionName, *UniqueId);
			}
		}
	}
}

APlayerState* APlayerState::Duplicate()
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = Instigator;
	SpawnInfo.bNoCollisionFail = true;
	APlayerState* NewPlayerState = GetWorld()->SpawnActor<APlayerState>(GetClass(), SpawnInfo );
	CopyProperties(NewPlayerState);
	return NewPlayerState;
}

void APlayerState::SeamlessTravelTo(APlayerState* NewPlayerState)
{
	CopyProperties(NewPlayerState);
	NewPlayerState->bOnlySpectator = bOnlySpectator;
}


bool APlayerState::IsPrimaryPlayer() const
{
	return true;
}

void APlayerState::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( APlayerState, Score );

	DOREPLIFETIME( APlayerState, PlayerName );
	DOREPLIFETIME( APlayerState, bIsSpectator );
	DOREPLIFETIME( APlayerState, bOnlySpectator );
	DOREPLIFETIME( APlayerState, bFromPreviousLevel );
	DOREPLIFETIME( APlayerState, StartTime );

	DOREPLIFETIME_CONDITION( APlayerState, Ping,		COND_SkipOwner );

	DOREPLIFETIME_CONDITION( APlayerState, PlayerId,	COND_InitialOnly );
	DOREPLIFETIME_CONDITION( APlayerState, bIsABot,		COND_InitialOnly );
	DOREPLIFETIME_CONDITION( APlayerState, bIsInactive, COND_InitialOnly );
	DOREPLIFETIME_CONDITION( APlayerState, UniqueId,	COND_InitialOnly );
}
