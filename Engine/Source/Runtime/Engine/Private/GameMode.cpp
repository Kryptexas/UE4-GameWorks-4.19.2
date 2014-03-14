// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GameMode.cpp: AGameMode c++ code.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineLevelScriptClasses.h"
#include "Online.h"

DEFINE_LOG_CATEGORY_STATIC(LogGameMode, Log, All);

AGameMode::AGameMode(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP
		.DoNotCreateDefaultSubobject(TEXT("Sprite"))
	)
{
	bDelayedStart = false;

	// One-time initialization
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	HUDClass = AHUD::StaticClass();
	bWaitingToStartMatch = false;
	bPauseable = true;
	DefaultPawnClass = ADefaultPawn::StaticClass();
	PlayerControllerClass = APlayerController::StaticClass();
	SpectatorClass = ASpectatorPawn::StaticClass();
	EngineMessageClass = UEngineMessage::StaticClass();
	GameStateClass = AGameState::StaticClass();
	CurrentID = 1;
	PlayerStateClass = APlayerState::StaticClass();
	MinRespawnDelay = 1.0f;
}


FString AGameMode::GetNetworkNumber()
{
	UNetDriver* NetDriver = GetWorld()->GetNetDriver();
	return NetDriver ? NetDriver->LowLevelGetNetworkNumber() : FString(TEXT(""));
}


void AGameMode::SwapPlayerControllers(APlayerController* OldPC, APlayerController* NewPC)
{
	if (OldPC != NULL && !OldPC->IsPendingKill() && NewPC != NULL && !NewPC->IsPendingKill() && OldPC->Player != NULL)
	{
		// move the Player to the new PC
		UPlayer* Player = OldPC->Player;
		NewPC->NetPlayerIndex = OldPC->NetPlayerIndex; //@warning: critical that this is first as SetPlayer() may trigger RPCs
		NewPC->SetPlayer(Player);
		NewPC->NetConnection = OldPC->NetConnection;
		NewPC->CopyRemoteRoleFrom( OldPC );

		// send destroy event to old PC immediately if it's local
		if (Cast<ULocalPlayer>(Player))
		{
			GetWorld()->DestroyActor(OldPC);
		}
		else
		{
			OldPC->PendingSwapConnection = Cast<UNetConnection>(Player);
			//@note: at this point, any remaining RPCs sent by the client on the old PC will be discarded
			// this is consistent with general owned Actor destruction,
			// however in this particular case it could easily be changed
			// by modifying UActorChannel::ReceivedBunch() to account for PendingSwapConnection when it is setting bNetOwner
		}
	}
	else
	{
		UE_LOG(LogGameMode, Warning, TEXT("SwapPlayerControllers(): Invalid OldPC, invalid NewPC, or OldPC has no Player!"));
	}
}

void AGameMode::ForceClearUnpauseDelegates( AActor* PauseActor )
{
	if ( PauseActor != NULL )
	{
		bool bUpdatePausedState = false;
		for ( int32 PauserIdx = Pausers.Num() - 1; PauserIdx >= 0; PauserIdx-- )
		{
			FCanUnpause& CanUnpauseDelegate = Pausers[PauserIdx];
			if ( CanUnpauseDelegate.GetUObject() == PauseActor )
			{
				Pausers.RemoveAt(PauserIdx);
				bUpdatePausedState = true;
			}
		}

		// if we removed some CanUnpause delegates, we may be able to unpause the game now
		if ( bUpdatePausedState )
		{
			ClearPause();
		}

		APlayerController* PC = Cast<APlayerController>(PauseActor);
		AWorldSettings * WorldSettings = GetWorldSettings();
		if ( PC != NULL && PC->PlayerState != NULL && WorldSettings != NULL && WorldSettings->Pauser == PC->PlayerState )
		{
			// try to find another player to be the worldsettings's Pauser
			for( FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator )
			{
				APlayerController* Player = *Iterator;
				if (Player->PlayerState != NULL
					&&	Player->PlayerState != PC->PlayerState
					&&	!Player->IsPendingKillPending() && !Player->PlayerState->IsPendingKillPending())
				{
					WorldSettings->Pauser = Player->PlayerState;
					break;
				}
			}

			// if it's still pointing to the original player's PlayerState, clear it completely
			if ( WorldSettings->Pauser == PC->PlayerState )
			{
				WorldSettings->Pauser = NULL;
			}
		}
	}
}

void AGameMode::InitGame( const FString& MapName, const FString& Options, FString& ErrorMessage )
{
	// save Options for future use
	OptionsString = Options;

	UClass* const SessionClass = GetGameSessionClass();
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = Instigator;
	GameSession = GetWorld()->SpawnActor<AGameSession>( SessionClass, SpawnInfo );
	GameSession->InitOptions(Options);

	if (GetNetMode() != NM_Standalone)
	{
		FOnlineSessionSettings* SessionSettings = NULL;
		IOnlineSessionPtr Sessions = Online::GetSessionInterface();
		if (Sessions.IsValid())
		{
			SessionSettings = Sessions->GetSessionSettings(GameSessionName);
		}

		if (!SessionSettings && !GameSession->ProcessAutoLogin())
		{
			GameSession->RegisterServer();
		}
	}
}

bool AGameMode::SetPause(APlayerController* PC, FCanUnpause CanUnpauseDelegate)
{
	if ( AllowPausing(PC) )
	{
		// Don't add the delegate twice (no need)
		if (Pausers.Find(CanUnpauseDelegate) == INDEX_NONE)
		{
			// Not in the list so add it for querying
			Pausers.Add(CanUnpauseDelegate);
		}
		// Let the first one in "own" the pause state
		AWorldSettings * WorldSettings = GetWorldSettings();
		if (WorldSettings->Pauser == NULL)
		{
			WorldSettings->Pauser = PC->PlayerState;
		}
		return true;
	}
	return false;
}

void AGameMode::RestartGame()
{
	if ( GameSession->CanRestartGame() )
	{
		if (bGameRestarted)
		{
			return;
		}
		bGameRestarted = true;

		GetWorld()->ServerTravel("?Restart",GetTravelType());
	}
}

void AGameMode::ReturnToMainMenuHost()
{
	if (GameSession)
	{
		GameSession->ReturnToMainMenuHost();
	}
}

void AGameMode::PostLogin( APlayerController* NewPlayer )
{
	UWorld* World = GetWorld();

	// update player count
	if (NewPlayer->PlayerState->bOnlySpectator)
	{
		NumSpectators++;
	}
	else if (World->IsInSeamlessTravel() || NewPlayer->HasClientLoadedCurrentWorld())
	{
		NumPlayers++;
	}
	else
	{
		NumTravellingPlayers++;
	}

	// save network address for re-associating with reconnecting player, after stripping out port number
	FString Address = NewPlayer->GetPlayerNetworkAddress();
	int32 pos = Address.Find(TEXT(":"), ESearchCase::CaseSensitive);
	NewPlayer->PlayerState->SavedNetworkAddress = (pos > 0) ? Address.Left(pos) : Address;

	// check if this player is reconnecting and already has PlayerState
	FindInactivePlayer(NewPlayer);

	StartNewPlayer(NewPlayer);
	NewPlayer->ClientCapBandwidth(NewPlayer->Player->CurrentNetSpeed);
	if(World->NetworkManager)
	{
		World->NetworkManager->UpdateNetSpeeds(true);	 // @TODO ONLINE - Revisit LAN netspeeds
	}

	GenericPlayerInitialization(NewPlayer);

	if (NewPlayer->PlayerState->bOnlySpectator)
	{
		NewPlayer->ClientGotoState(NAME_Spectating);
	}

	if (GameSession)
	{
		GameSession->PostLogin(NewPlayer);
	}

	// add the player to any matinees running so that it gets in on any cinematics already running, etc
	TArray<AMatineeActor*> AllMatineeActors;
	World->GetMatineeActors(AllMatineeActors);
	for (int32 i = 0; i < AllMatineeActors.Num(); i++)
	{
		AllMatineeActors[i]->AddPlayerToDirectorTracks(NewPlayer);
	}

	bool HidePlayer=false, HideHUD=false, DisableMovement=false, DisableTurning=false;

	//Check to see if we should start in cinematic mode (matinee movie capture)
	if(ShouldStartInCinematicMode(HidePlayer, HideHUD, DisableMovement, DisableTurning))
	{
		NewPlayer->SetCinematicMode(true, HidePlayer, HideHUD, DisableMovement, DisableTurning);
	}

	// Tell the player to enable voice by default or use the push to talk method
	NewPlayer->ClientEnableNetworkVoice(!GameSession->RequiresPushToTalk());
}

bool AGameMode::ShouldStartInCinematicMode(bool& OutHidePlayer,bool& OutHideHUD,bool& OutDisableMovement,bool& OutDisableTurning)
{
	bool StartInCinematicMode = false;
	if(GEngine->bStartWithMatineeCapture)
	{
		GConfig->GetBool( TEXT("MatineeCreateMovieOptions"), TEXT("CinematicMode"), StartInCinematicMode, GEditorUserSettingsIni );
		if(StartInCinematicMode)
		{
			GConfig->GetBool( TEXT("MatineeCreateMovieOptions"), TEXT("DisableMovement"), OutDisableMovement, GEditorUserSettingsIni );
			GConfig->GetBool( TEXT("MatineeCreateMovieOptions"), TEXT("DisableTurning"), OutDisableTurning, GEditorUserSettingsIni );
			GConfig->GetBool( TEXT("MatineeCreateMovieOptions"), TEXT("HidePlayer"), OutHidePlayer, GEditorUserSettingsIni );
			GConfig->GetBool( TEXT("MatineeCreateMovieOptions"), TEXT("HideHUD"), OutHideHUD, GEditorUserSettingsIni );
			return StartInCinematicMode;
		}
	}
	return StartInCinematicMode;
}


void AGameMode::SetPlayerDefaults(APawn* PlayerPawn)
{
	PlayerPawn->SetPlayerDefaults();
}

void AGameMode::Logout( AController* Exiting )
{
	APlayerController* PC = Cast<APlayerController>(Exiting);
	if ( PC != NULL )
	{
		RemovePlayerControllerFromPlayerCount(PC);

		if (GameSession)
		{
			GameSession->NotifyLogout(PC);
		}

		// @TODO ONLINE - Revisit LAN netspeeds
		UWorld* World = GetWorld();
		if (World && World->NetworkManager)
		{
			World->NetworkManager->UpdateNetSpeeds(true);
		}
	}
}

void AGameMode::InitGameState()
{
	GameState->GameModeClass = GetClass();
	GameState->ReceivedGameModeClass();
	GameState->SpectatorClass = SpectatorClass;
	GameState->ReceivedSpectatorClass();
}


AActor* AGameMode::FindPlayerStart( AController* Player, const FString& IncomingName )
{
	// if incoming start is specified, then just use it
	if( !IncomingName.IsEmpty() )
	{
		for( FActorIterator It(GetWorld()); It; ++It )
		{
			APlayerStart* Start = Cast<APlayerStart>(*It);
			if ( Start && Start->PlayerStartTag == FName(*IncomingName) )
			{
				return Start;
			}
		}
	}

	// always pick StartSpot at start of match
	if ( ShouldSpawnAtStartSpot(Player) )
	{
		return Player->StartSpot.Get();
	}

	AActor* BestStart = ChoosePlayerStart(Player);

	if ( (BestStart == NULL) && (Player == NULL) )
	{
		// no playerstart found
		UE_LOG(LogGameMode, Log, TEXT("Warning - PATHS NOT DEFINED or NO PLAYERSTART with positive rating"));

		// Search all loaded levels for possible player start object
		for ( int32 LevelIndex = 0; LevelIndex < GetWorld()->GetNumLevels(); ++LevelIndex )
		{
			ULevel* Level = GetWorld()->GetLevel( LevelIndex );
			for ( int32 ActorIndex = 0; ActorIndex < Level->Actors.Num(); ++ActorIndex )
			{
				AActor* NavObject = Level->Actors[ ActorIndex ];
				if ( NavObject )
				{
					BestStart = NavObject;
					break;
				}
			}

			if (BestStart != NULL)
			{
				break;
			}
		}
	}

	return BestStart;
}

void AGameMode::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	GetWorldTimerManager().SetTimer(this, &AGameMode::DefaultTimer, GetWorldSettings()->GetEffectiveTimeDilation(), true);
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = Instigator;

	// Fallback to default GameState if none was specified.
	if (GameStateClass == NULL)
	{
		UE_LOG(LogGameMode, Warning, TEXT("No GameStateClass was specified in %s (%s)"), *GetName(), *GetClass()->GetName());
		GameStateClass = AGameState::StaticClass();
	}

	GameState = GetWorld()->SpawnActor<AGameState>(GameStateClass, SpawnInfo);
	GetWorld()->GameState = GameState;
	GameState->AuthorityGameMode = this;

	// Only need NetworkManager for servers in net games
	GetWorld()->NetworkManager = GetWorldSettings()->GameNetworkManagerClass ? GetWorld()->SpawnActor<AGameNetworkManager>(GetWorldSettings()->GameNetworkManagerClass, SpawnInfo ) : NULL;

	InitGameState();
}

void AGameMode::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	bWaitingToStartMatch = true;
	bMatchIsInProgress = false;

	if(GameSession != NULL)
	{
		GameSession->StartPendingMatch();
	}

	// validate default classes
// 	check(DefaultPawnClass && DefaultPawnClass->IsChildOf(APawn::StaticClass()));
// 	check(PlayerControllerClass && PlayerControllerClass->IsChildOf(APlayerController::StaticClass()));
// 	check(SpectatorClass && SpectatorClass->IsChildOf(ASpectatorPawn::StaticClass()));
// 	check(EngineMessageClass && EngineMessageClass->IsChildOf(UEngineMessage::StaticClass()));
// 	check(GameStateClass && GameStateClass->IsChildOf(AGameState::StaticClass()));
// 	check(PlayerStateClass && PlayerStateClass->IsChildOf(APlayerState::StaticClass()));
}


void AGameMode::RestartPlayer(AController* NewPlayer)
{
	if ( bWaitingToStartMatch || NewPlayer == NULL || NewPlayer->IsPendingKillPending() )
	{
		return;
	}
	AActor* StartSpot = FindPlayerStart(NewPlayer);

	// if a start spot wasn't found,
	if (StartSpot == NULL)
	{
		// check for a previously assigned spot
		if (NewPlayer->StartSpot != NULL)
		{
			StartSpot = NewPlayer->StartSpot.Get();
			UE_LOG(LogGameMode, Warning, TEXT("Player start not found, using last start spot"));
		}
		else
		{
			// otherwise abort
			UE_LOG(LogGameMode, Warning, TEXT("Player start not found, failed to restart player"));
			return;
		}
	}
	// try to create a pawn to use of the default class for this player
	if (NewPlayer->GetPawn() == NULL)
	{
		NewPlayer->SetPawn(SpawnDefaultPawnFor(NewPlayer, StartSpot));
	}
	if (NewPlayer->GetPawn() == NULL)
	{
		UE_LOG(LogGameMode, Warning, TEXT("failed to spawn player at %s"), *StartSpot->GetName());
		NewPlayer->FailedToSpawnPawn();
	}
	else
	{
		// initialize and start it up
		InitStartSpot(StartSpot, NewPlayer);

		// @todo: this was related to speedhack code, which is disabled.
		/*
		if ( NewPlayer->GetAPlayerController() )
		{
			NewPlayer->GetAPlayerController()->TimeMargin = -0.1f;
		}
		*/
		NewPlayer->Possess(NewPlayer->GetPawn());
		
		// set initial control rotation to player start's rotation
		NewPlayer->ClientSetRotation(NewPlayer->GetPawn()->GetActorRotation(), true);

		FRotator NewControllerRot = StartSpot->GetActorRotation();
		NewControllerRot.Roll = 0.f;
		NewPlayer->SetControlRotation( NewControllerRot );

		SetPlayerDefaults(NewPlayer->GetPawn());
	}

#if !WITH_PHYSX
	if (NewPlayer->GetPawn() != NULL)
	{
		UCharacterMovementComponent* CharacterMovement = Cast<UCharacterMovementComponent>(NewPlayer->GetPawn()->GetMovementComponent());
		if (CharacterMovement)
		{
			CharacterMovement->bCheatFlying = true;
			CharacterMovement->SetMovementMode(MOVE_Flying);
		}
	}
#endif	//!WITH_PHYSX
}

void AGameMode::InitStartSpot(AActor* StartSpot, AController* NewPlayer)
{
}

void AGameMode::StartMatch()
{
	/**
	 * Tells all of the currently connected clients to register with arbitration.
	 * The clients will call back to the server once they have done so, which
	 * will tell this state to see if it is time for the server to register with
	 * arbitration.
	 */
	if (!bMatchIsInProgress && GameSession->HandleStartMatch())
	{
		return;
	}

	if (bWaitingToStartMatch)
	{
		GameSession->EndPendingMatch();
	}

	bWaitingToStartMatch = false;

	// start human players first
	for( FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator )
	{
		APlayerController* PlayerController = *Iterator;
		if( ( PlayerController->GetPawn() == NULL) && PlayerController->CanRestartPlayer() )
		{
			RestartPlayer(PlayerController);
		}
	}

	// Notify all clients that the match has begun
	if(GetWorld()->GameState != NULL)
	{
		GetWorld()->GameState->StartMatch();
	}

	// Make sure level streaming is up to date before triggering NotifyMatchStarted
	GetWorld()->FlushLevelStreaming();

	// fire off any level startup events
	GetWorldSettings()->NotifyMatchStarted();

	// if passed in bug info, send player to right location
	FString BugLocString = ParseOption(OptionsString, TEXT("BugLoc"));
	FString BugRotString = ParseOption(OptionsString, TEXT("BugRot"));
	if( !BugLocString.IsEmpty() || !BugRotString.IsEmpty() )
	{
		for( FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator )
		{
			APlayerController* PlayerController = *Iterator;
			if( PlayerController->CheatManager != NULL )
			{
				//`log( "BugLocString:" @ BugLocString );
				//`log( "BugRotString:" @ BugRotString );

				PlayerController->CheatManager->BugItGoString( BugLocString, BugRotString );
			}
		}
	}
}


void AGameMode::ResetLevel()
{
	UE_LOG(LogGameMode, Log, TEXT("Reset %s"), *GetName());

	// Reset ALL controllers first
	for( FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator )
	{
		AController* Controller = *Iterator;
		APlayerController* PlayerController = Cast<APlayerController>(Controller);
		if ( PlayerController )
		{
			PlayerController->ClientReset();
		}
		Controller->Reset();
	}

	// Reset all actors (except controllers, the GameMode, and any other actors specified by ShouldReset())
	for (FActorIterator It(GetWorld()); It; ++It)
	{
		AActor* A = *It;
		if(	A && !A->IsPendingKill() && A != this && !A->IsA<AController>() && ShouldReset(A))
		{
			A->Reset();
		}
	}

	// reset the GameMode
	Reset();

	// Notify the level script that the level has been reset
	ALevelScriptActor* LevelScript = GetWorld()->GetLevelScriptActor();
	if( LevelScript )
	{
		LevelScript->LevelReset();
	}
}


void AGameMode::HandleSeamlessTravelPlayer(AController*& C)
{
	UE_LOG(LogGameMode, Log, TEXT(">> GameMode::HandleSeamlessTravelPlayer: %s "), *C->GetName());

	APlayerController* PC = Cast<APlayerController>(C);
	if (PC != NULL && PC->GetClass() != PlayerControllerClass)
	{
		if (PC->Player != NULL)
		{
			// we need to spawn a new PlayerController to replace the old one
			APlayerController* NewPC = SpawnPlayerController(PC->GetFocalLocation(), PC->GetControlRotation());
			if (NewPC == NULL)
			{
				UE_LOG(LogGameMode, Warning, TEXT("Failed to spawn new PlayerController for %s (old class %s)"), *PC->GetHumanReadableName(), *PC->GetClass()->GetName());
				PC->Destroy();
				return;
			}
			else
			{
				PC->CleanUpAudioComponents();
				PC->SeamlessTravelTo(NewPC);
				NewPC->SeamlessTravelFrom(PC);
				SwapPlayerControllers(PC, NewPC);
				PC = NewPC;
				C = NewPC;
			}
		}
		else
		{
			PC->Destroy();
		}
	}
	else
	{
		// clear out data that was only for the previous game
		C->PlayerState->Reset();
		// create a new PlayerState and copy over info; this is necessary because the old GameMode may have used a different PlayerState class
		APlayerState* OldPlayerState = C->PlayerState;
		C->InitPlayerState();
		OldPlayerState->SeamlessTravelTo(C->PlayerState);
		// we don"t need the old PlayerState anymore
		//@fixme: need a way to replace PlayerStates that doesn't cause incorrect "player left the game"/"player entered the game" messages
		OldPlayerState->Destroy();
	}

	// Find a start spot->
	AActor* StartSpot = FindPlayerStart(C);

	if (StartSpot == NULL)
	{
		UE_LOG(LogGameMode, Warning, TEXT("Could not find a starting spot"));
	}
	else
	{
		FRotator StartRotation(0, StartSpot->GetActorRotation().Yaw, 0);
		C->SetInitialLocationAndRotation(StartSpot->GetActorLocation(), StartRotation);
	}

	C->StartSpot = StartSpot;

	if (PC != NULL)
	{
		// Track the last completed seamless travel for the player
		PC->LastSeamlessTravelCount = PC->SeamlessTravelCount;

		PC->CleanUpAudioComponents();

		if (PC->PlayerCameraManager == NULL)
		{
			PC->SpawnPlayerCameraManager();
		}

		SetSeamlessTravelViewTarget(PC);
		if (PC->PlayerState->bOnlySpectator)
		{
			PC->StartSpectatingOnly();

			// NumSpectators is incremented in PostSeamlessTravel
		}
		else
		{
			NumPlayers++;
			NumTravellingPlayers--;
			PC->bPlayerIsWaiting = true;
			PC->ChangeState(NAME_Spectating);
		}

		// tell client what hud class to use
		PC->ClientSetHUD(HUDClass);
	}
	else
	{
		NumBots++;
	}

	GenericPlayerInitialization(C);

	// RestartPlayer if match has started. This fixes bugs if the seamless travel player loads slow and
	// the server calls StartMatch before he loads.
	if (!bWaitingToStartMatch && PC)
	{
		RestartPlayer(PC);
	}

	UE_LOG(LogGameMode, Log, TEXT("<< GameMode::HandleSeamlessTravelPlayer: %s"), *C->GetName());
}

void AGameMode::SetSeamlessTravelViewTarget(APlayerController* PC)
{
	PC->SetViewTarget(PC);
}


void AGameMode::ProcessServerTravel(const FString& URL, bool bAbsolute)
{
#if WITH_SERVER_CODE
	bLevelChange = true;

	// force an old style load screen if the server has been up for a long time so that TimeSeconds doesn't overflow and break everything
	bool bSeamless = (bUseSeamlessTravel && GetWorld()->TimeSeconds < 172800.0f); // 172800 seconds == 48 hours

	FString NextMap;
	if (URL.ToUpper().Contains(TEXT("?RESTART")))
	{
		NextMap = GetOutermost()->GetName();
	}
	else
	{
		int32 OptionStart = URL.Find(TEXT("?"));
		if (OptionStart == INDEX_NONE)
		{
			NextMap = URL;
		}
		else
		{
			NextMap = URL.Left(OptionStart);
		}
	}
	
	// Handle short package names (convienience code so that short map names
	// can still be specified in the console).
	if (FPackageName::IsShortPackageName(NextMap))
	{
		UE_LOG(LogGameMode, Fatal, TEXT("ProcessServerTravel: %s: Short package names are not supported."), *URL);
		return;
	}
	
	FGuid NextMapGuid = UEngine::GetPackageGuid(FName(*NextMap));

	// Notify clients we're switching level and give them time to receive.
	FString URLMod = URL;
	APlayerController* LocalPlayer = ProcessClientTravel(URLMod, NextMapGuid, bSeamless, bAbsolute);

	UE_LOG(LogGameMode, Log, TEXT("ProcessServerTravel: %s"), *URL);
	UWorld* World = GetWorld(); 
    check(World);
	World->NextURL = URL;
	ENetMode NetMode = GetNetMode();

	if (bSeamless)
	{
		World->SeamlessTravel(World->NextURL, bAbsolute);
		World->NextURL = TEXT("");
	}
	// Switch immediately if not networking.
	else if (NetMode != NM_DedicatedServer && NetMode != NM_ListenServer)
	{
		World->NextSwitchCountdown = 0.0f;
	}
#endif // WITH_SERVER_CODE
}


void AGameMode::GetSeamlessTravelActorList(bool bToEntry, TArray<AActor*>& ActorList)
{
	UWorld* World = GetWorld();
	// always keep PlayerStates, so that after we restart we can keep players on the same team, etc
	for (int32 i = 0; i < World->GameState->PlayerArray.Num(); i++)
	{
		World->GameState->PlayerArray[i]->bFromPreviousLevel = true;
		World->GameState->PlayerArray[i]->ForceNetUpdate();
		ActorList.Add(World->GameState->PlayerArray[i]);
	}

	if (bToEntry)
	{
		// keep general game state until we transition to the final destination
		ActorList.Add(World->GameState);
	}

	if (GameSession)
	{
		ActorList.Add(GameSession);
	}
}

void AGameMode::SetBandwidthLimit(float AsyncIOBandwidthLimit)
{
	GAsyncIOBandwidthLimit = AsyncIOBandwidthLimit;
}

void AGameMode::InitNewPlayer(AController* NewPlayer, const FString& Options) {}

bool AGameMode::MustSpectate(APlayerController* NewPlayer)
{
	return NewPlayer->PlayerState->bOnlySpectator;
}


APlayerController* AGameMode::Login(const FString& Portal, const FString& Options, const TSharedPtr<FUniqueNetId>& UniqueId, FString& ErrorMessage)
{
	ErrorMessage = GameSession->ApproveLogin(Options);
	if ( !ErrorMessage.IsEmpty() )
	{
		return NULL;
	}

	APlayerController* NewPlayer = SpawnPlayerController(FVector::ZeroVector, FRotator::ZeroRotator);

	// Handle spawn failure.
	if( NewPlayer == NULL)
	{
		UE_LOG(LogGameMode, Log, TEXT("Couldn't spawn player controller of class %s"), PlayerControllerClass ? *PlayerControllerClass->GetName() : TEXT("NULL"));
		ErrorMessage = FString::Printf(TEXT("Failed to spawn player controller"));
		return NULL;
	}

	// Customize incoming player based on URL options
	InitNewPlayer(NewPlayer, Options);

	// Find a start spot.
	AActor* const StartSpot = FindPlayerStart( NULL, Portal );
	if( StartSpot == NULL )
	{
		ErrorMessage = FString::Printf(TEXT("Failed to find PlayerStart"));
		return NULL;
	}

	FRotator InitialControllerRot = StartSpot->GetActorRotation();
	InitialControllerRot.Roll = 0.f;
	NewPlayer->SetInitialLocationAndRotation(StartSpot->GetActorLocation(), InitialControllerRot);
	NewPlayer->StartSpot = StartSpot;

	// Register the player with the session
	GameSession->RegisterPlayer(NewPlayer, UniqueId, HasOption(Options, TEXT("bIsFromInvite")));
	
	// Init player's name
	FString InName = ParseOption( Options, TEXT("Name")).Left(20);
	if( InName.IsEmpty() )
	{
		InName=FString::Printf(TEXT("%s%i"), *DefaultPlayerName, NewPlayer->PlayerState->PlayerId);
	}
	ChangeName( NewPlayer, InName, false );

	// Set up spectating
	bool bSpectator = FCString::Stricmp(*ParseOption( Options, TEXT("SpectatorOnly") ), TEXT("1")) == 0;
	if ( bSpectator || MustSpectate(NewPlayer) )
	{
		NewPlayer->StartSpectatingOnly();
		return NewPlayer;
	}

	if (bDelayedStart)
	{
		NewPlayer->bPlayerIsWaiting = true;
		NewPlayer->ChangeState(NAME_Spectating);
		return NewPlayer;
	}

	return NewPlayer;
}


void AGameMode::DisplayDebug(UCanvas* Canvas, const TArray<FName>& DebugDisplay, float& YL, float& YPos)
{
	Canvas->SetDrawColor(255,255,255);
}


void AGameMode::Reset()
{
	Super::Reset();
	InitGameState();
}


bool AGameMode::ShouldReset(AActor* ActorToReset)
{
	return true;
}

void AGameMode::GameEnding() {}

void AGameMode::PlayerSwitchedToSpectatorOnly(APlayerController* PC)
{
	RemovePlayerControllerFromPlayerCount(PC);
	NumSpectators++;
}

void AGameMode::RemovePlayerControllerFromPlayerCount(APlayerController* PC)
{
	if ( PC )
	{
		if ( PC->PlayerState->bOnlySpectator )
		{
			NumSpectators--;
		}
		else
		{
			if ( GetWorld()->IsInSeamlessTravel() || PC->HasClientLoadedCurrentWorld() )
			{
				NumPlayers--;
			}
			else
			{
				NumTravellingPlayers--;
			}
		}
	}
}

int32 AGameMode::GetNumPlayers()
{
	return NumPlayers + NumTravellingPlayers;
}



void AGameMode::ClearPause()
{
	if ( !AllowPausing() && Pausers.Num() > 0 )
	{
		UE_LOG(LogGameMode, Log, TEXT("Clearing list of UnPause delegates for %s because game type is not pauseable"), *GetFName().ToString());
		Pausers.Empty();
	}

	for (int32 Index = 0; Index < Pausers.Num(); Index++)
	{
		FCanUnpause CanUnpauseCriteriaMet = Pausers[Index];
		if( CanUnpauseCriteriaMet.IsBound() )
		{
			bool Result = CanUnpauseCriteriaMet.Execute();
			if (Result)
			{
				Pausers.RemoveAt(Index--,1);
			}
		}
		else
		{
			Pausers.RemoveAt(Index--,1);
		}
	}

	// Clear the pause state if the list is empty
	if ( Pausers.Num() == 0 )
	{
		GetWorldSettings()->Pauser = NULL;
	}
}

bool AGameMode::GrabOption( FString& Options, FString& Result )
{
	if( Options.Left(1)==TEXT("?") )
	{
		// Get result.
		Result = Options.Mid(1, MAX_int32);
		if( Result.Contains(TEXT("?")) )
		{
			Result =  Result.Left( Result.Find(TEXT("?")) );
		}

		// Update options.
		Options = Options.Mid(1, MAX_int32);
		if( Options.Contains(TEXT("?")) )
		{
			Options =  Options.Mid( Options.Find(TEXT("?")) , MAX_int32);
		}
		else
		{
			Options = TEXT("");
		}

		return true;
	}
	else return false;
}

void AGameMode::GetKeyValue( const FString& Pair, FString& Key, FString& Value )
{
	const int32 EqualSignIndex = Pair.Find(TEXT("="));
	if( EqualSignIndex != INDEX_NONE )
	{
		Key = Pair.Left(EqualSignIndex);
		Value = Pair.Mid(EqualSignIndex + 1, MAX_int32);
	}
	else
	{
		Key = Pair;
		Value = TEXT("");
	}
}

FString AGameMode::ParseOption( const FString& Options, const FString& InKey )
{
	FString OptionsMod = Options;
	FString Pair, Key, Value;
	while( GrabOption( OptionsMod, Pair ) )
	{
		GetKeyValue( Pair, Key, Value );
		if( FCString::Stricmp(*Key, *InKey) == 0 )
			return Value;
	}
	return TEXT("");
}

bool AGameMode::HasOption( const FString& Options, const FString& InKey )
{
	FString OptionsMod = Options;
	FString Pair, Key, Value;
	while( GrabOption( OptionsMod, Pair ) )
	{
		GetKeyValue( Pair, Key, Value );
		if( FCString::Stricmp(*Key, *InKey) == 0 )
		{
			return true;
		}
	}
	return false;
}

int32 AGameMode::GetIntOption( const FString& Options, const FString& ParseString, int32 CurrentValue)
{
	FString InOpt = ParseOption( Options, ParseString );
	if ( !InOpt.IsEmpty() )
	{
		return FCString::Atoi(*InOpt);
	}
	return CurrentValue;
}

FString AGameMode::GetDefaultGameClassPath(const FString& MapName, const FString& Options, const FString& Portal)
{
	return GetClass()->GetPathName();
}


TSubclassOf<AGameMode> AGameMode::SetGameMode(const FString& MapName, const FString& Options, const FString& Portal)
{
	return GetClass();
}

TSubclassOf<AGameSession> AGameMode::GetGameSessionClass() const
{
	return AGameSession::StaticClass();
}

void AGameMode::NotifyPendingConnectionLost() {}

APlayerController* AGameMode::ProcessClientTravel( FString& FURL, FGuid NextMapGuid, bool bSeamless, bool bAbsolute )
{
	// We call PreClientTravel directly on any local PlayerPawns (ie listen server)
	APlayerController* LocalPlayerController = NULL;
	for( FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator )
	{
		APlayerController* PlayerController = *Iterator;
		if ( Cast<UNetConnection>(PlayerController->Player) != NULL )
		{
			// remote player
			PlayerController->ClientTravel(FURL, TRAVEL_Relative, bSeamless, NextMapGuid);
		}
		else
		{
			// local player
			LocalPlayerController = PlayerController;
			PlayerController->PreClientTravel(FURL, bAbsolute ? TRAVEL_Absolute : TRAVEL_Relative, bSeamless);
		}
	}

	return LocalPlayerController;
}

void AGameMode::PreLogin(const FString& Options, const FString& Address, const TSharedPtr<FUniqueNetId>& UniqueId, FString& ErrorMessage)
{
	ErrorMessage = GameSession->ApproveLogin(Options);
}

APlayerController* AGameMode::SpawnPlayerController(FVector const& SpawnLocation, FRotator const& SpawnRotation)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = Instigator;	
	return GetWorld()->SpawnActor<APlayerController>(PlayerControllerClass, SpawnLocation, SpawnRotation, SpawnInfo );
}

UClass* AGameMode::GetDefaultPawnClassForController(AController* InController)
{
	return DefaultPawnClass;
}

APawn* AGameMode::SpawnDefaultPawnFor(AController* NewPlayer, AActor* StartSpot)
{
	// don't allow pawn to be spawned with any pitch or roll
	FRotator StartRotation(ForceInit);
	StartRotation.Yaw = StartSpot->GetActorRotation().Yaw;
	FVector StartLocation = StartSpot->GetActorLocation();

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = Instigator;	
	APawn* ResultPawn = GetWorld()->SpawnActor<APawn>(GetDefaultPawnClassForController(NewPlayer), StartLocation, StartRotation, SpawnInfo );
	if ( ResultPawn == NULL )
	{
		UE_LOG(LogGameMode, Warning, TEXT("Couldn't spawn Pawn of type %s at %s"), *GetNameSafe(DefaultPawnClass), *StartSpot->GetName());
	}
	return ResultPawn;
}

void AGameMode::ReplicateStreamingStatus(APlayerController* PC)
{
	// don't do this for local players or players after the first on a splitscreen client
	if (Cast<ULocalPlayer>(PC->Player) == NULL && Cast<UChildConnection>(PC->Player) == NULL)
	{
		// if we've loaded levels via CommitMapChange() that aren't normally in the StreamingLevels array, tell the client about that
		if (GetWorld()->CommittedPersistentLevelName != NAME_None)
		{
			PC->ClientPrepareMapChange(GetWorld()->CommittedPersistentLevelName, true, true);
			// tell the client to commit the level immediately
			PC->ClientCommitMapChange();
		}

		if (GetWorld()->StreamingLevels.Num() > 0)
		{
	 		// Tell the player controller the current streaming level status
	 		for (int32 LevelIndex = 0; LevelIndex < GetWorld()->StreamingLevels.Num(); LevelIndex++)
	 		{
				// streamingServer
				ULevelStreaming* TheLevel = GetWorld()->StreamingLevels[LevelIndex];

				if( TheLevel != NULL )
				{
					const ULevel* LoadedLevel = TheLevel->GetLoadedLevel();
					
					UE_LOG(LogGameMode, Log, TEXT("levelStatus: %s %i %i %i %s %i"),
						*TheLevel->PackageName.ToString(),
						TheLevel->bShouldBeVisible,
						LoadedLevel && LoadedLevel->bIsVisible,
						TheLevel->bShouldBeLoaded,
						*GetNameSafe(LoadedLevel),
						TheLevel->bHasLoadRequestPending);

	 				PC->ClientUpdateLevelStreamingStatus(
	 					TheLevel->PackageName,
	 					TheLevel->bShouldBeLoaded,
	 					TheLevel->bShouldBeVisible,
	 					TheLevel->bShouldBlockOnLoad,
						TheLevel->GetLODIndex(GetWorld()));
	 			}
			}
	 		PC->ClientFlushLevelStreaming();
		}

		// if we're preparing to load different levels using PrepareMapChange() inform the client about that now
		if (GetWorld()->PreparingLevelNames.Num() > 0)
		{
			for (int32 LevelIndex = 0; LevelIndex < GetWorld()->PreparingLevelNames.Num(); LevelIndex++)
			{
				PC->ClientPrepareMapChange(GetWorld()->PreparingLevelNames[LevelIndex], LevelIndex == 0, LevelIndex == GetWorld()->PreparingLevelNames.Num() - 1);
			}
			// DO NOT commit these changes yet - we'll send that when we're done preparing them
		}
	}
}

void AGameMode::GenericPlayerInitialization(AController* C)
{
	APlayerController* PC = Cast<APlayerController>(C);
	if (PC != NULL)
	{
		ReplicateStreamingStatus(PC);
	}
}

void AGameMode::StartNewPlayer(APlayerController* NewPlayer)
{
	// tell client what hud class to use
	NewPlayer->ClientSetHUD(HUDClass);

	if (!bDelayedStart)
	{
		// start match, or let player enter, immediately
		if ( bWaitingToStartMatch )
		{
			StartMatch();
		}
		else
		{
			RestartPlayer(NewPlayer);
		}

		if (NewPlayer->GetPawn() != NULL)
		{
			NewPlayer->GetPawn()->ClientSetRotation(NewPlayer->GetPawn()->GetActorRotation());
		}
	}
}

void AGameMode::PreExit() {}



bool AGameMode::CanSpectate( APlayerController* Viewer, APlayerState* ViewTarget )
{
	return true;
}

void AGameMode::ChangeName( AController* Other, const FString& S, bool bNameChange )
{
	if( !S.IsEmpty() )
	{
		Other->PlayerState->SetPlayerName(S);
	}
}

void AGameMode::SendPlayer( APlayerController* aPlayer, const FString& FURL )
{
	aPlayer->ClientTravel( FURL, TRAVEL_Relative );
}


bool AGameMode::GetTravelType()
{
	return false;
}

void AGameMode::Broadcast( AActor* Sender, const FString& Msg, FName Type )
{
	APlayerState* SenderPlayerState = NULL;
	if ( Cast<APawn>(Sender) != NULL )
	{
		SenderPlayerState = Cast<APawn>(Sender)->PlayerState;
	}
	else if ( Cast<AController>(Sender) != NULL )
	{
		SenderPlayerState = Cast<AController>(Sender)->PlayerState;
	}

	for( FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator )
	{
		(*Iterator)->ClientTeamMessage( SenderPlayerState, Msg, Type );
	}
}


void AGameMode::BroadcastLocalized( AActor* Sender, TSubclassOf<ULocalMessage> Message, int32 Switch, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject )
{
	for( FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator )
	{
		(*Iterator)->ClientReceiveLocalizedMessage( Message, Switch, RelatedPlayerState_1, RelatedPlayerState_2, OptionalObject );
	}
}

void AGameMode::PerformEndGameHandling()
{
	GameState->EndGame();
	GameSession->EndGame();
}

bool AGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	return ( Player != NULL && Player->StartSpot != NULL );
}

void AGameMode::AddPlayerStart(APlayerStart* NewPlayerStart)
{
	PlayerStarts.AddUnique(NewPlayerStart);
}

void AGameMode::RemovePlayerStart(APlayerStart* RemovedPlayerStart)
{
	PlayerStarts.Remove(RemovedPlayerStart);
}

AActor* AGameMode::ChoosePlayerStart( AController* Player )
{
	// Find first player start
	APlayerStart* FoundPlayerStart = NULL;
	for (int32 PlayerStartIndex = 0; PlayerStartIndex < PlayerStarts.Num(); ++PlayerStartIndex)
	{
		APlayerStart* PlayerStart = PlayerStarts[PlayerStartIndex];

		if( Cast<APlayerStartPIE>( PlayerStart ) != NULL )
		{
			// Always prefer the first "Play from Here" PlayerStart, if we find one while in PIE mode
			FoundPlayerStart = PlayerStart;
			break;
		}
		else if( PlayerStart != NULL && FoundPlayerStart == NULL )
		{
			FoundPlayerStart = PlayerStart;
		}
	}
	return FoundPlayerStart;
}

bool AGameMode::PlayerCanRestartGame( APlayerController* aPlayer )
{
	return true;
}

bool AGameMode::PlayerCanRestart( APlayerController* aPlayer )
{
	return true;
}

bool AGameMode::AllowCheats(APlayerController* P)
{
	return ( GetNetMode() == NM_Standalone || GIsEditor ); // Always allow cheats in editor (PIE now supports networking)
}

bool AGameMode::AllowPausing( APlayerController* PC )
{
	return bPauseable || GetNetMode() == NM_Standalone;
}

void AGameMode::PreCommitMapChange(const FString& PreviousMapName, const FString& NextMapName) {}

void AGameMode::PostCommitMapChange() {}

void AGameMode::AddInactivePlayer(APlayerState* PlayerState, APlayerController* PC)
{
	// don't store if it's an old PlayerState from the previous level or if it's a spectator
	if (!PlayerState->bFromPreviousLevel && !PlayerState->bOnlySpectator)
	{
		APlayerState* NewPlayerState = PlayerState->Duplicate();
		GetWorld()->GameState->RemovePlayerState(NewPlayerState);

		// make PlayerState inactive
		NewPlayerState->SetReplicates(false);

		// delete after 5 minutes
		NewPlayerState->SetLifeSpan( 300 );

		// On console, we have to check the unique net id as network address isn't valid
		bool bIsConsole = GEngine->IsConsoleBuild();

		// make sure no duplicates
		for (int32 i=0; i<InactivePlayerArray.Num(); i++)
		{
			APlayerState* CurrentPlayerState = InactivePlayerArray[i];
			if ( (CurrentPlayerState == NULL) || CurrentPlayerState->IsPendingKill() ||
				(!bIsConsole && (CurrentPlayerState->SavedNetworkAddress == NewPlayerState->SavedNetworkAddress)))
			{
				InactivePlayerArray.RemoveAt(i,1);
				i--;
			}
		}
		InactivePlayerArray.Add(NewPlayerState);

		// cap at 16 saved PlayerStates
		if ( InactivePlayerArray.Num() > 16 )
		{
			InactivePlayerArray.RemoveAt(0, InactivePlayerArray.Num() - 16);
		}
	}

	PlayerState->Destroy();
}


bool AGameMode::FindInactivePlayer(APlayerController* PC)
{
	// don't bother for spectators
	if (PC->PlayerState->bOnlySpectator)
	{
		return false;
	}

	// On console, we have to check the unique net id as network address isn't valid
	bool bIsConsole = GEngine->IsConsoleBuild();

	FString NewNetworkAddress = PC->PlayerState->SavedNetworkAddress;
	FString NewName = PC->PlayerState->PlayerName;
	for (int32 i=0; i<InactivePlayerArray.Num(); i++)
	{
		APlayerState* CurrentPlayerState = InactivePlayerArray[i];
		if ( (CurrentPlayerState == NULL) || CurrentPlayerState->IsPendingKill() )
		{
			InactivePlayerArray.RemoveAt(i,1);
			i--;
		}
		else if ( (bIsConsole /** @TODO ONLINE - Add back uniqueid compare */) ||
			(!bIsConsole && (FCString::Stricmp(*CurrentPlayerState->SavedNetworkAddress, *NewNetworkAddress) == 0) && (FCString::Stricmp(*CurrentPlayerState->PlayerName, *NewName) == 0)) )
		{
			// found it!
			APlayerState* OldPlayerState = PC->PlayerState;
			PC->PlayerState = CurrentPlayerState;
			PC->PlayerState->SetOwner(PC);
			PC->PlayerState->SetReplicates(true);
			PC->PlayerState->SetLifeSpan( 0.0f );
			OverridePlayerState(PC, OldPlayerState);
			GetWorld()->GameState->AddPlayerState(PC->PlayerState);
			InactivePlayerArray.RemoveAt(i,1);
			OldPlayerState->bIsInactive = true;
			// Set the uniqueId to NULL so it will not kill the player's registration 
			// in UnregisterPlayerWithSession()
			OldPlayerState->SetUniqueId(NULL);
			OldPlayerState->Destroy();
			return true;
		}
	}
	return false;
}

void AGameMode::OverridePlayerState(APlayerController* PC, APlayerState* OldPlayerState)
{
	PC->PlayerState->OverrideWith(OldPlayerState);
}

void AGameMode::PostSeamlessTravel()
{
	if ( GameSession != NULL )
	{
		GameSession->PostSeamlessTravel();
	}

	// We have to make a copy of the controller list, since the code after this will destroy
	// and create new controllers in the world's list
	TArray<TAutoWeakObjectPtr<class AController> >	OldControllerList;
	for (auto It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		OldControllerList.Add(*It);
	}

	// handle players that are already loaded
	for( FConstControllerIterator Iterator = OldControllerList.CreateConstIterator(); Iterator; ++Iterator )
	{
		AController* Controller = *Iterator;
		if (Controller->PlayerState)
		{
			APlayerController* PlayerController = Cast<APlayerController>(Controller);
			if (PlayerController == NULL)
			{
				HandleSeamlessTravelPlayer(Controller);
			}
			else
			{
				if (Controller->PlayerState->bOnlySpectator)
				{
					// The spectator count must be incremented here, instead of in HandleSeamlessTravelPlayer,
					// as otherwise spectators can 'hide' from player counters, by making HasClientLoadedCurrentWorld return false
					NumSpectators++;
				}
				else
				{
					NumTravellingPlayers++;
				}
				if (PlayerController->HasClientLoadedCurrentWorld())
				{
					HandleSeamlessTravelPlayer(Controller);
				}
			}
		}
	}

	if ( ReadyToStartMatch() )
	{
		StartMatch();
	}
}

bool AGameMode::ReadyToStartMatch()
{
	return (bWaitingToStartMatch && NumPlayers + NumBots > 0);
}

void AGameMode::MatineeCancelled() {}

void AGameMode::OnEngineHasLoaded() {}


FString AGameMode::StaticGetFullGameClassName(FString const& Str)
{
	// look to see if this should be remapped from a shortname to full class name
	const AGameMode* const DefaultGameMode = GetDefault<AGameMode>();
	if (DefaultGameMode)
	{
		int32 const NumAliases = DefaultGameMode->GameModeClassAliases.Num();
		for (int32 Idx=0; Idx<NumAliases; ++Idx)
		{
			const FGameClassShortName& Alias = DefaultGameMode->GameModeClassAliases[Idx];
			if ( Str == Alias.ShortName )
			{
				// switch GameClassName to the full name
				return Alias.GameClassName;
			}
		}
	}

	return Str;
}

void AGameMode::DefaultTimer()
{	
}

