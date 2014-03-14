// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Multiplayer game session.
 */

#pragma once
#include "Runtime/Online/OnlineSubsystem/Public/OnlineSubsystemTypes.h"
#include "GameSession.generated.h"

UCLASS(config=Game, notplaceable)
class ENGINE_API AGameSession : public AInfo
{
	GENERATED_UCLASS_BODY()

	/** Maximum number of spectators allowed by this server. */
	UPROPERTY(globalconfig)
	int32 MaxSpectators;

	/** Maximum number of players allowed by this server. */
	UPROPERTY(globalconfig)
	int32 MaxPlayers;

	/** Maximum number of splitscreen players to allow from one connection */
	UPROPERTY(globalconfig)
	uint8 MaxSplitscreensPerConnection;

    /** Is voice enabled always or via a push to talk keybinding */
	UPROPERTY(globalconfig)
	bool bRequiresPushToTalk;

	/** The options to apply for dedicated server when it starts to register */
	UPROPERTY()
	FString ServerOptions;

	/** SessionName local copy from PlayerState class.  should really be define in this class, but need to address replication issues */
	UPROPERTY()
	FName SessionName;

	/** Initialize options based on passed in options string */
	virtual void InitOptions( const FString& Options );

	/** @return A new unique player ID */
	int32 GetNextPlayerID();

	/**
	 * Chance to save objects from GC during seamless travel
	 * @param bToEntry are we traveling to or from the transition map
	 * @param ActorList list of actors to preserve
	 */
	virtual void GetSeamlessTravelActorList(bool bToEntry, TArray<AActor*>& ActorList) {}

	//=================================================================================
	// LOGIN

	/** 
	 * Allow an online service to process a login if specified on the commandline with -login/-pass
	 * @return true if login is in progress, false otherwise
	 */
	virtual bool ProcessAutoLogin();

    /** Delegate triggered on auto login completion */
	virtual void OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);

    /** Delegate triggered on auto login success */ 
	virtual void OnLoginChanged(int32 LocalUserNum);

	/** 
	 * Called from GameMode.PreLogin() and Login().
	 * @param	Options	The URL options (e.g. name/spectator) the player has passed
	 * @return	Non-empty Error String if player not approved
	 */
	virtual FString ApproveLogin(const FString& Options);

	/**
	 * Register a player with the online service session
	 * @param NewPlayer player to register
	 * @param UniqueId uniqueId they sent over on Login
	 * @param bWasFromInvite was this from an invite
	 */
	virtual void RegisterPlayer(APlayerController* NewPlayer, const TSharedPtr<FUniqueNetId>& UniqueId, bool bWasFromInvite);

	/**
	 * Called by GameMode::PostLogin to give session code chance to do work after PostLogin
	 * @param NewPlayer player logging in
	 */
	virtual void PostLogin(APlayerController* NewPlayer);

	/** @return true if there is no room on the server for an additional player */
	virtual bool AtCapacity(bool bSpectator);

	//=================================================================================
	// LOGOUT

	/** Called when a PlayerController logs out of game. */
	virtual void NotifyLogout(class APlayerController* PC);

	/** Unregister a player from the online service session	 */
	virtual void UnregisterPlayer(APlayerController* ExitingPlayer);

	/** Forcibly remove KickedPlayer from the server */
	virtual bool KickPlayer(class APlayerController* KickedPlayer, const FString& KickReason);

	/** Gracefully tell all clients then local players to return to lobby */
	virtual void ReturnToMainMenuHost();

	/** 
	* called after a seamless level transition has been completed on the *new* GameMode
	 * used to reinitialize players already in the game as they won't have *Login() called on them
	 */
	virtual void PostSeamlessTravel();

	//=================================================================================
	// SESSION INFORMATION

	/** Restart the session	 */
	virtual void Restart() {}

	/** Allow a dedicated server a chance to register itself with an online service */
	virtual void RegisterServer();

	/**
	 * Update session join parameters
	 *
	 * @param SessionName name of session to update
	 * @param bPublicSearchable can the game be found via matchmaking
	 * @param bAllowInvites can you invite friends
	 * @param bJoinViaPresence anyone who can see you can join the game
	 * @param bJoinViaPresenceFriendsOnly can only friends actively join your game 
	 */
	virtual void UpdateSessionJoinability(FName SessionName, bool bPublicSearchable, bool bAllowInvites, bool bJoinViaPresence, bool bJoinViaPresenceFriendsOnly);

    /**
     * Does the session require push to talk
     * @return true if a push to talk keybinding is required or if voice is always enabled
     */
	virtual bool RequiresPushToTalk() const { return bRequiresPushToTalk; }

	/** Dump session info to log for debugging.	  */
	virtual void DumpSessionState();

	//=================================================================================
	// MATCH INTERFACE

	/** Start a pending match */
	virtual void StartPendingMatch();

	/** End a pending match */
	virtual void EndPendingMatch();

	/** Called from GameMode.RestartGame(). */
	virtual bool CanRestartGame();

	/** End a game */
	virtual void EndGame();

	/** @RETURNS true if GameSession handled starting the match */
	virtual bool HandleStartMatch();

private:
	// Hidden functions that don't make sense to use on this class.
	HIDE_ACTOR_TRANSFORM_FUNCTIONS();
};

/** 
 * Returns the player controller associated with this net id
 * @param PlayerNetId the id to search for
 * @return the player controller if found, otherwise NULL
 */
ENGINE_API class APlayerController* GetPlayerControllerFromNetId(class UWorld* World, const FUniqueNetId& PlayerNetId);




