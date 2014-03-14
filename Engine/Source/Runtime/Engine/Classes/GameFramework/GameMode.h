// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameMode.generated.h"

//=============================================================================
//  GameMode defines the rules and mechanics of the game.  It is only 
//  instanced on the server and will never exist on the client.
//=============================================================================


/** Default delegate that provides an implementation for those that don't have special needs other than a toggle */
DECLARE_DELEGATE_RetVal( bool, FCanUnpause );

/** Configurable shortened aliases for GameMode classes.  For convenience when typing urls, for instance. */
USTRUCT()
struct FGameClassShortName
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString ShortName;

	UPROPERTY()
	FString GameClassName;
};

//=============================================================================
// The GameMode defines the game being played. It governs the game rules, scoring, what actors
// are allowed to exist in this game type, and who may enter the game.
// A GameMode actor is instantiated when the level is initialized for gameplay in
// C++ UGameEngine::LoadMap().  The class of this GameMode actor is determined by
// (in order) either the URL ?game=xxx, or the
// DefaultGameMode entry in the game's .ini file (in the /Script/Engine.Engine section).
// The GameMode used can be overridden in the GameMode function SetGameMode(), called
// on the game class picked by the above process.
//
//=============================================================================
UCLASS(config=Game, notplaceable, BlueprintType, Blueprintable, hidecategories=(Info, Rendering, MovementReplication, Replication, Actor))
class ENGINE_API AGameMode : public AInfo
{
	GENERATED_UCLASS_BODY()

public:
	/** Whether the match is currently in progress */
	UPROPERTY()
	uint32 bMatchIsInProgress:1;

	/** Whether the game is pauseable. */
	UPROPERTY()
	uint32 bPauseable:1;    

	/** Match has not yet started - waiting for players to join and be ready. */
	UPROPERTY()
	uint32 bWaitingToStartMatch:1; 

	/** Flag to make sure RestartGame() only gets executed once. */
	UPROPERTY()
	uint32 bGameRestarted:1; 

	/** Level transition in progress. */
	UPROPERTY()
	uint32 bLevelChange:1;    

	/** Save options string and parse it when needed */
	UPROPERTY()
	FString OptionsString;

	/** The default pawn class used by players. */
	UPROPERTY(EditAnywhere, noclear, BlueprintReadWrite, Category=GameMode)
	TSubclassOf<class APawn>  DefaultPawnClass;

	/** HUD class this game uses. */
	UPROPERTY(EditAnywhere, noclear, BlueprintReadWrite, Category=GameMode, meta=(DisplayName="HUD Class"))
	TSubclassOf<class AHUD>  HUDClass;    

	/** Current number of spectators. */
	UPROPERTY()
	int32 NumSpectators;    

	/** Current number of human players. */
	UPROPERTY()
	int32 NumPlayers;    

	/** number of non-human players (AI controlled but participating as a player). */
	UPROPERTY()
	int32 NumBots;    

	/** Minimum time before player can respawn after dying. */
	UPROPERTY()
	float MinRespawnDelay;

	/** Game Session handles login approval, arbitration, online game interface */
	UPROPERTY()
	class AGameSession* GameSession;

	/** Number of players that are still traveling from a previous map */
	UPROPERTY()
	int32 NumTravellingPlayers;

	/** Used to assign unique PlayerIDs to each PlayerState. */
	UPROPERTY()
	int32 CurrentID;    

	/** The default player name assigned to players that join with no name specified. */
	UPROPERTY(localized)
	FString DefaultPlayerName;

	/** Array of available player starts. */
	UPROPERTY()
	TArray<class APlayerStart*> PlayerStarts;

	/** Contains strings describing localized game agnostic messages. */
	UPROPERTY()
	TSubclassOf<class ULocalMessage> EngineMessageClass;

	/** The class of PlayerController to spawn for players logging in. */
	UPROPERTY(EditAnywhere, noclear, BlueprintReadWrite, Category=GameMode)
	TSubclassOf<class APlayerController> PlayerControllerClass;

	/** The pawn class used by the PlayerController for players when spectating. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=GameMode)
	TSubclassOf<class ASpectatorPawn> SpectatorClass;

	/** A PlayerState of this class will be associated with every player to replicate relevant player information to all clients. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=GameMode, AdvancedDisplay)
	TSubclassOf<class APlayerState> PlayerStateClass;

	/** Class of GameState associated with this GameMode. */
	UPROPERTY(EditAnywhere, noclear, BlueprintReadWrite, Category=GameMode)
	TSubclassOf<class AGameState> GameStateClass;

	/** GameState is used to replicate game state relevant properties to all clients. */
	UPROPERTY(Transient)
	class AGameState* GameState;

	/** Helper template to returns the current GameState casted to the desired type. */
	template< class T >
	T* GetGameState() const
	{
		return Cast<T>(GameState);
	}

	/** PlayerStates of players who have disconnected from the server (saved in case they reconnect) */
	UPROPERTY()
	TArray<class APlayerState*> InactivePlayerArray;    

	/** The list of delegates to check before unpausing a game */
	TArray<FCanUnpause> Pausers;

	/** 
	 * perform map travels using SeamlessTravel() which loads in the background and doesn't disconnect clients
	 * @see World::SeamlessTravel()
	 */
	UPROPERTY()
	uint32 bUseSeamlessTravel:1;

	/** Tracks whether the server can travel due to a critical network error. */
	UPROPERTY()
	uint32 bHasNetworkError:1;

	/** Whether the game should immediately start when the player logs in. */
	uint32 bDelayedStart:1;

protected:

	/** Handy alternate short names for GameMode classes (e.g. "DM" could be an alias for "MyProject.MyGameModeMP_DM". */
	UPROPERTY(config)
	TArray<struct FGameClassShortName> GameModeClassAliases;

public:
	/** Does end of game handling for the online layer. */
	virtual void PerformEndGameHandling();

	/** Alters the synthetic bandwidth limit for a running game. */
	UFUNCTION(exec)
	virtual void SetBandwidthLimit(float AsyncIOBandwidthLimit);

	// Begin AActor interface
	virtual void PreInitializeComponents() OVERRIDE;
	virtual void PostInitializeComponents() OVERRIDE;
	virtual void AddPlayerStart(APlayerStart* NewPlayerStart);
	virtual void RemovePlayerStart(APlayerStart* RemovedPlayerStart);
	virtual void DisplayDebug(class UCanvas* Canvas, const TArray<FName>& DebugDisplay, float& YL, float& YPos) OVERRIDE;
	virtual void Reset() OVERRIDE;
	// End AActor interface

	/** 
	 * @return true if ActorToReset should have Reset() called on it while restarting the game, 
	 *		   false if the GameMode will manually reset it or if the actor does not need to be reset
	 */
	virtual bool ShouldReset(AActor* ActorToReset);

	/** Called when StartSpot is selected for spawning NewPlayer to allow optional initialization. */
	virtual void InitStartSpot(class AActor* StartSpot, AController* NewPlayer);

	/** Resets level by calling Reset() on all actors */
	virtual void ResetLevel();

	/**
	 *	Check to see if we should start in cinematic mode (e.g. matinee movie capture).
	 * 	@param	OutHidePlayer		Whether to hide the player
	 *	@param	OutHideHud			Whether to hide the HUD		
	 *	@param	OutDisableMovement	Whether to disable movement
	 * 	@param	OutDisableTurning	Whether to disable turning
	 *	@return	bool			true if we should turn on cinematic mode, 
	 *							false if if we should not.
	 */
	bool ShouldStartInCinematicMode(bool& OutHidePlayer, bool& OutHideHud, bool& OutDisableMovement, bool& OutDisableTurning);

	/** Called right before exiting or performing a server travel. */
	virtual void GameEnding();

	/**
	 * Initialize the GameState actor with default settings
	 * called during PreInitializeComponents() of the GameMode after a GameState has been spawned
	 * as well as during Reset()
	 */
	virtual void InitGameState();

	/** Get local address */
	virtual FString GetNetworkNumber();
	
	/** Will remove the controller from the correct count then add them to the spectator count. **/
	void PlayerSwitchedToSpectatorOnly(APlayerController* PC);

	/** Removes the passed in player controller from the correct count for player/spectator/tranistioning **/
	void RemovePlayerControllerFromPlayerCount(APlayerController* PC);

	/** Total number of players */
	virtual int32 GetNumPlayers();

	/**
	 * Adds the delegate to the list if the player Controller has the right to pause
	 * the game. The delegate is called to see if it is ok to unpause the game, e.g.
	 * the reason the game was paused has been cleared.
	 * @param PC the player Controller to check for admin privs
	 * @param CanUnpauseDelegate the delegate to query when checking for unpause
	 */
	virtual bool SetPause(APlayerController* PC, FCanUnpause CanUnpauseDelegate = FCanUnpause());

	/**
	 * Checks the list of delegates to determine if the pausing can be cleared. If
	 * the delegate says it's ok to unpause, that delegate is removed from the list
	 * and the rest are checked. The game is considered unpaused when the list is
	 * empty.
	 */
	virtual void ClearPause();

	/**
	 * Forcibly removes an object's CanUnpause delegates from the list of pausers.  If any of the object's CanUnpause delegate
	 * handlers were in the list, triggers a call to ClearPause().
	 *
	 * Called when the player controller is being destroyed to prevent the game from being stuck in a paused state when a PC that
	 * paused the game is destroyed before the game is unpaused.
	 */
	void ForceClearUnpauseDelegates( AActor* PauseActor );

	//=========================================================================
	// URL Parsing
	/** Grab the next option from a string. */
	bool GrabOption( FString& Options, FString& ResultString );

	/** Break up a key=value pair into its key and value. */
	void GetKeyValue( const FString& Pair, FString& Key, FString& Value );

	/* Find an option in the options string and return it. */
	FString ParseOption( const FString& Options, const FString& InKey );

	/** HasOption - return true if the option is specified on the command line. */
	bool HasOption( const FString& Options, const FString& InKey );

	/** Search array of options for ParseString and return the index or CurrentValue if not found*/
	int32 GetIntOption( const FString& Options, const FString& ParseString, int32 CurrentValue);
	//=========================================================================

	/** 
	 * @return the full path to the optimal GameMode class to use for the specified map and options
	 * this is used for preloading cooked packages, etc. and therefore doesn't need to include any fallbacks
	 * as SetGameMode() will be called later to actually find/load the desired class
	 */
	FString GetDefaultGameClassPath(const FString& MapName, const FString& Options, const FString& Portal);

	/** 
	 * @return the class of GameMode to spawn for the game on the specified map and the specified options
	 * this function should include any fallbacks in case the desired class can't be found
	 */
	TSubclassOf<AGameMode> SetGameMode(const FString& MapName, const FString& Options, const FString& Portal);

	/** @return GameSession class to use for this game  */
	virtual TSubclassOf<class AGameSession> GetGameSessionClass() const;

	/**
	 * Initialize the game.
	 * The GameMode's InitGame() event is called before any other scripts (including
	 * PreInitializeComponents() ), and is used by the GameMode to initialize parameters and spawn
	 * its helper classes.
	 * @warning: this is called before actors' PreInitializeComponents.
	 */
	virtual void InitGame( const FString& MapName, const FString& Options, FString& ErrorMessage );

	/** Called when a connection closes before getting to PostLogin() */
	virtual void NotifyPendingConnectionLost();

	/* Optional handling of ServerTravel for network games. */
	virtual void ProcessServerTravel(const FString& URL, bool bAbsolute = false);

	/**
	 * Notifies all clients to travel to the specified URL.
	 *
	 * @param	URL				a string containing the mapname (or IP address) to travel to, along with option key/value pairs
	 * @param	NextMapGuid		the GUID of the server's version of the next map
	 * @param	bSeamless		indicates whether the travel should use seamless travel or not.
	 * @param	bAbsolute		indicates which type of travel the server will perform (i.e. TRAVEL_Relative or TRAVEL_Absolute)
	 */
	virtual APlayerController* ProcessClientTravel( FString& URL, FGuid NextMapGuid, bool bSeamless, bool bAbsolute );

	/** 
	 * Accept or reject a player attempting to join the server.  Fails login if you set the ErrorMessage to a non-empty string.
	 * PreLogin is called before Login.  Significant game time may pass before Login is called, especially if content is downloaded.
	 *
	 * @param	Options					The URL options (e.g. name/spectator) the player has passed
	 * @param	Address					The network address of the player
	 * @param	UniqueId				The unique id the player has passed to the server
	 * @param	ErrorMessage			When set to a non-empty value, the player will be rejected using the error message set
	 */
	virtual void PreLogin(const FString& Options, const FString& Address, const TSharedPtr<class FUniqueNetId>& UniqueId, FString& ErrorMessage);

	/** If login is successful, returns a new PlayerController to associate with this player. Login fails if ErrorMessage string is set. */
	virtual APlayerController* Login(const FString& Portal, const FString& Options, const TSharedPtr<class FUniqueNetId>& UniqueId, FString& ErrorMessage);

	/** Called after a successful login.  This is the first place it is safe to call replicated functions on the PlayerAController. */
	virtual void PostLogin( APlayerController* NewPlayer );

	/** Spawns a PlayerController at the specified location; split out from Login()/HandleSeamlessTravelPlayer() for easier overriding */
	virtual APlayerController* SpawnPlayerController(FVector const& SpawnLocation, FRotator const& SpawnRotation);

	/** @Returns true if NewPlayer may only join the server as a spectator. */
	virtual bool MustSpectate(APlayerController* NewPlayer);

	/* Start the game - inform all actors that the match is starting, and spawn player pawns. */
	virtual void StartMatch();

	/** returns default pawn class for given controller */
	virtual UClass* GetDefaultPawnClassForController(AController* InController);

	/**
	 * @param	NewPlayer - Controller for whom this pawn is spawned
	 * @param	StartSpot - PlayerStart at which to spawn pawn
	 * @return	a pawn of the default pawn class
	 */
	virtual APawn* SpawnDefaultPawnFor(AController* NewPlayer, class AActor* StartSpot);

	/** replicates the current level streaming status to the given PlayerController */
	virtual void ReplicateStreamingStatus(APlayerController* PC);

	/**
	 * handles all player initialization that is shared between the travel methods
	 * (i.e. called from both PostLogin() and HandleSeamlessTravelPlayer())
	 */
	virtual void GenericPlayerInitialization(AController* C);

	/** start match, or let player enter, immediately */
	virtual void StartNewPlayer(APlayerController* NewPlayer);

	/** Engine is shutting down. */
	virtual void PreExit();

	/** Called when a Controller with a PlayerState leaves the match. */
	virtual void Logout( AController* Exiting );

	/**
	 * Make sure pawn properties are back to default
	 * Also a good place to modify them on spawn
	 */
	virtual void SetPlayerDefaults(APawn* PlayerPawn);

	/** Return whether Viewer is allowed to spectate from the point of view of ViewTarget. */
	virtual bool CanSpectate( APlayerController* Viewer, APlayerState* ViewTarget );

	/* Use reduce damage for teamplay modifications, etc. */
	virtual void ChangeName( AController* Other, const FString& S, bool bNameChange );

	/* Send a player to a URL.*/
	virtual void SendPlayer( APlayerController* aPlayer, const FString& URL );

	/** @return true if we want to travel_absolute */
	virtual bool GetTravelType();

	/** Restart the game */
	virtual void RestartGame();

	/** Return to main menu */
	virtual void ReturnToMainMenuHost();

	/** Broadcast a string to all players. */
	virtual void Broadcast( AActor* Sender, const FString& Msg, FName Type = NAME_None );

	/**
	 * Broadcast a localized message to all players.
	 * Most message deal with 0 to 2 related PlayerStates.
	 * The LocalMessage class defines how the PlayerState's and optional actor are used.
	 */
	virtual void BroadcastLocalized( AActor* Sender, TSubclassOf<ULocalMessage> Message, int32 Switch = 0, APlayerState* RelatedPlayerState_1 = NULL, APlayerState* RelatedPlayerState_2 = NULL, UObject* OptionalObject = NULL );

	/** @return true if the given Controller StartSpot property should be used as the spawn location for its Pawn */
	virtual bool ShouldSpawnAtStartSpot(AController* Player);

	/** 
	 * Return the 'best' player start for this player to start from.
	 * @param Player - is the AController for whom we are choosing a playerstart
	 * @param IncomingName - specifies the tag of a Playerstart to use
	 * @returns Actor chosen as player start (usually a PlayerStart)
	 */
	virtual class AActor* FindPlayerStart( AController* Player, const FString& IncomingName = TEXT("") );

	/** 
	* Return the 'best' player start for this player to start from.  
	* Default implementation just returns the first PlayerStart found.
	* @param Player is the controller for whom we are choosing a playerstart
	* @returns AActor chosen as player start (usually a PlayerStart)
	 */
	virtual class AActor* ChoosePlayerStart( AController* Player );

	/** @return true if player can restart the game */
	virtual bool PlayerCanRestartGame( APlayerController* aPlayer );

	/** @return true if player can be restarted */
	virtual bool PlayerCanRestart( APlayerController* aPlayer );

	/** @return true if player is allowed to access the cheats */
	virtual bool AllowCheats(APlayerController* P);

	/** @return	true if the player is allowed to pause the game. */
	virtual bool AllowPausing( APlayerController* PC = NULL );

	/**
	 * Called from CommitMapChange before unloading previous level
	 * @param PreviousMapName - Name of the previous persistent level
	 * @param NextMapName - Name of the persistent level being streamed to
	 */
	virtual void PreCommitMapChange(const FString& PreviousMapName, const FString& NextMapName);

	/** Called from CommitMapChange after unloading previous level and loading new level+sublevels */
	virtual void PostCommitMapChange();

	/** Add PlayerState to the inactive list, remove from the active list */
	virtual void AddInactivePlayer(APlayerState* PlayerState, APlayerController* PC);

	/** Attempt to find and associate an inactive PlayerState with entering PC.  
	  * @Returns true if a PlayerState was found and associated with PC. */
	virtual bool FindInactivePlayer(APlayerController* PC);

	/** Override PC's PlayerState with the values in OldPlayerState. */
	virtual void OverridePlayerState(APlayerController* PC, APlayerState* OldPlayerState);

	/** 
	 * called on server during seamless level transitions to get the list of Actors that should be moved into the new level
	 * PlayerControllers, Role < ROLE_Authority Actors, and any non-Actors that are inside an Actor that is in the list
	 * (i.e. Object.Outer == Actor in the list)
	 * are all autmoatically moved regardless of whether they're included here
	 * only dynamic actors in the PersistentLevel may be moved (this includes all actors spawned during gameplay)
	 * this is called for both parts of the transition because actors might change while in the middle (e.g. players might join or leave the game)
	 * @see also PlayerController::GetSeamlessTravelActorList() (the function that's called on clients)
	 * @param bToEntry true if we are going from old level -> entry, false if we are going from entry -> new level
	 * @param ActorList (out) list of actors to maintain
	 */
	virtual void GetSeamlessTravelActorList(bool bToEntry, TArray<AActor*>& ActorList);

	/** used to swap a viewport/connection's PlayerControllers when seamless travelling and the new GameMode's
	 * controller class is different than the previous
	 * includes network handling
	 * @param OldPC - the old PC that should be discarded
	 * @param NewPC - the new PC that should be used for the player
	 */
	virtual void SwapPlayerControllers(APlayerController* OldPC, APlayerController* NewPC);

	/** called after a seamless level transition has been completed on the *new* GameMode
	 * used to reinitialize players already in the game as they won't have *Login() called on them
	 */
	virtual void PostSeamlessTravel();

	/** @return true if ready to Start Match */
	virtual bool ReadyToStartMatch();

	/** 
	 * Handles reinitializing players that remained through a seamless level transition
	 * called from C++ for players that finished loading after the server
	 * @param C the Controller to handle
	 */
	virtual void HandleSeamlessTravelPlayer(AController*& C);

	/** SetViewTarget of player control on server change */
	virtual void SetSeamlessTravelViewTarget(APlayerController* PC);

	/** Called when this PC is in cinematic mode, and its matinee is canceled by the user. */
	virtual void MatineeCancelled();

	/** Alters the synthetic bandwidth limit for a running game **/
	virtual void OnEngineHasLoaded();

	/** Does end of game handling for the online layer */
	virtual void RestartPlayer(class AController* NewPlayer);

	/** Given a string, return a fully-qualified GameMode class name */
	static FString StaticGetFullGameClassName(FString const& Str);

	/** Called periodically, overridden by subclasses */
	virtual void DefaultTimer();	

protected:
	/** Customize incoming player based on URL options  */
	virtual void InitNewPlayer(AController* NewPlayer, const FString& Options);

private:
	// Hidden functions that don't make sense to use on this class.
	HIDE_ACTOR_TRANSFORM_FUNCTIONS();
};



