// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// LocalPlayer
//=============================================================================

#pragma once
#include "LocalPlayer.generated.h"

struct ENGINE_API FGameWorldContext
{
	FGameWorldContext();
	FGameWorldContext( const AActor* InActor );
	FGameWorldContext( const class ULocalPlayer* InLocalPlayer );
	FGameWorldContext( const class APlayerController* InPlayerController );
	FGameWorldContext( const FGameWorldContext& InWidgetWorldContext );

	/* Returns the world context. */
	UWorld* GetWorld() const;

	/* Returns the local player. */
	class ULocalPlayer* GetLocalPlayer() const;

	/* Returns the player controller. */
	class APlayerController* GetPlayerController() const;

	/** Is this GameWorldContext initialized? */
	bool IsValid() const;

private:	

	/* Set the local player. */
	void SetLocalPlayer( const class ULocalPlayer* InLocalPlayer );

	/* Set the local player via a player controller. */
	void SetPlayerController( const class APlayerController* InPlayerController );

	TWeakObjectPtr<class ULocalPlayer>		LocalPlayer;
};

UCLASS(HeaderGroup=Network, Within=Engine, config=Engine, transient)
class ENGINE_API ULocalPlayer : public UPlayer
{
	GENERATED_UCLASS_BODY()

	/** The controller ID which this player accepts input from. */
	UPROPERTY()
	int32 ControllerId;

	/** The master viewport containing this player's view. */
	UPROPERTY()
	class UGameViewportClient* ViewportClient;

	/** The coordinates for the upper left corner of the master viewport subregion allocated to this player. 0-1 */
	UPROPERTY()
	FVector2D Origin;

	/** The size of the master viewport subregion allocated to this player. 0-1 */
	UPROPERTY()
	FVector2D Size;

	/** The location of the player's view the previous frame. */
	UPROPERTY(transient)
	FVector LastViewLocation;

	/** How to constrain perspective viewport FOV */
	UPROPERTY(config)
	TEnumAsByte<enum EAspectRatioAxisConstraint> AspectRatioAxisConstraint;

	/** set when we've sent a split join request */
	UPROPERTY(VisibleAnywhere, transient, Category=LocalPlayer)
	uint32 bSentSplitJoin:1;

private:
	FSceneViewStateReference ViewState;
	FSceneViewStateReference StereoViewState;

	/** Class to manage online services */
	UPROPERTY(transient)
	class UOnlineSession* OnlineSession;

	/** @return OnlineSession class to use for this player controller  */
	virtual TSubclassOf<UOnlineSession> GetOnlineSessionClass();

public:
	// UObject interface
	virtual void PostInitProperties() OVERRIDE;
	virtual void FinishDestroy() OVERRIDE;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	// End of UObject interface

	// FExec interface
	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd,FOutputDevice& Ar) OVERRIDE;
	// End of FExec interface

	/** 
	 * Exec command handlers
	 */

	bool HandleDNCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleExitCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandlePauseCommand( const TCHAR* Cmd, FOutputDevice& Ar, UWorld* InWorld );
	bool HandleListMoveBodyCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleListAwakeBodiesCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleListSimBodiesCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleMoveComponentTimesCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleListSkelMeshesCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleListPawnComponentsCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleExecCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleToggleDrawEventsCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleToggleStreamingVolumesCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleCancelMatineeCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	
	// UPlayer interface
	virtual void HandleDisconnect(class UWorld *World, class UNetDriver *NetDriver) OVERRIDE;
	// End of UPlayer interface

protected:
	/**
	 * Retrieve the viewpoint of this player.
	 * @param OutViewInfo - Upon return contains the view information for the player.
	 */
	void GetViewPoint(FMinimalViewInfo& OutViewInfo);

	/** @todo document */
	void ExecMacro( const TCHAR* Filename, FOutputDevice& Ar );

public:

	/**
	 * Get the world the players actor belongs to
	 *
	 * @return  Returns the world of the LocalPlayer's PlayerController. NULL if the LocalPlayer does not have a PlayerController
	 */
	UWorld* GetWorld() const;

	/**
	 * Calculate the view settings for drawing from this view actor
	 *
	 * @param	View - output view struct
	 * @param	OutViewLocation - output actor location
	 * @param	OutViewRotation - output actor rotation
	 * @param	Viewport - current client viewport
	 * @param	ViewDrawer - optional drawing in the view
	 * @param	StereoPass - whether we are drawing the full viewport, or a stereo left / right pass
	 */
	FSceneView* CalcSceneView(class FSceneViewFamily* ViewFamily,
		FVector& OutViewLocation, 
		FRotator& OutViewRotation, 
		FViewport* Viewport,
		class FViewElementDrawer* ViewDrawer = NULL,
		EStereoscopicPass StereoPass = eSSP_FULL );

	/**
	 * Called at creation time for internal setup
	 */
	virtual void PlayerAdded(class UGameViewportClient* InViewportClient, int32 InControllerID);

	/**
	 * Called when the player is removed from the viewport client
	 */
	virtual void PlayerRemoved();

	/**
	 * Create an actor for this player.
	 * @param URL - The URL the player joined with.
	 * @param OutError - If an error occurred, returns the error description.
	 * @param InWorld - World in which to spawn the play actor
	 * @return False if an error occurred, true if the play actor was successfully spawned.	 
	 */
	virtual bool SpawnPlayActor(const FString& URL,FString& OutError, UWorld* InWorld);
	
	/** Send a splitscreen join command to the server to allow a splitscreen player to connect to the game
	 * the client must already be connected to a server for this function to work
	 * @note this happens automatically for all viewports that exist during the initial server connect
	 * 	so it's only necessary to manually call this for viewports created after that
	 * if the join fails (because the server was full, for example) all viewports on this client will be disconnected
	 */
	virtual void SendSplitJoin();
	
	/**
	 * Change the ControllerId for this player; if the specified ControllerId is already taken by another player, changes the ControllerId
	 * for the other player to the ControllerId currently in use by this player.
	 *
	 * @param	NewControllerId		the ControllerId to assign to this player.
	 */
	void SetControllerId(int32 NewControllerId);

	/** 
	 * Retrieves this player's name/tag from the online subsystem
	 * if this function returns a non-empty string, the returned name will replace the "Name" URL parameter
	 * passed around in the level loading and connection code, which normally comes from DefaultEngine.ini
	 * 
	 * @return Name of player if specified (by onlinesubsystem or otherwise), Empty string otherwise
	 */
	virtual FString GetNickname() const;

	/** 
	 * Retrieves any game-specific login options for this player
	 * if this function returns a non-empty string, the returned option or options be added
	 * passed in to the level loading and connection code.  Options are in URL format,
	 * key=value, with multiple options concatenated together with an & between each key/value pair
	 * 
	 * @return URL Option or options for this game, Empty string otherwise
	 */
	virtual FString GetGameLoginOptions() const { return TEXT(""); }

	/** 
	 * Retrieves this player's unique net ID from the online subsystem 
	 *
	 * @return unique Id associated with this player
	 */
	TSharedPtr<class FUniqueNetId> GetUniqueNetId() const;

	/**
	 * This function will give you two points in Pixel Space that surround the World Space box.
	 *
	 * @param	ActorBox		The World Space Box
	 * @param	OutLowerLeft	The Lower Left corner of the Pixel Space box
	 * @param	OutUpperRight	The Upper Right corner of the pixel space box
	 * @return  False if there is no viewport, or if the box is behind the camera completely
	 */
	bool GetPixelBoundingBox(const FBox& ActorBox, FVector2D& OutLowerLeft, FVector2D& OutUpperRight, const FVector2D* OptionalAllotedSize = NULL);

	/**
	 * This function will give you a point in Pixel Space from a World Space position
	 *
	 * @param	InPoint		The point in world space
	 * @param	OutPoint	The point in pixel space
	 * @return  False if there is no viewport, or if the box is behind the camera completely
	 */
	bool GetPixelPoint(const FVector& InPoint, FVector2D& OutPoint, const FVector2D* OptionalAllotedSize = NULL);


	/**
	 * Helper function for deriving various bits of data needed for projection
	 *
	 * @param	Viewport				The ViewClient's viewport
     * @param	StereoPass			    Whether this is a full viewport pass, or a left/right eye pass
	 * @param	ProjectionData			The structure to be filled with projection data
	 * @return  False if there is no viewport, or if the Actor is null
	 */
	bool GetProjectionData(FViewport* Viewport, EStereoscopicPass StereoPass, FSceneViewProjectionData& ProjectionData);

	/**
	 * Called on world origin changes
	 *
	 * @param InOffset		Offset applied to current world origin
	 */
	void ApplyWorldOffset(FVector InOffset);
};

