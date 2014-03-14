// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// Controller, the base class of players or AI.
//
// Controllers are non-physical actors that can possess a Pawn to control
// its actions.  PlayerControllers are used by human players to control pawns, while
// AIControllers implement the artificial intelligence for the pawns they control.
// Controllers take control of a pawn using their Possess() method, and relinquish
// control of the pawn by calling UnPossess().
//
// Controllers receive notifications for many of the events occurring for the Pawn they
// are controlling.  This gives the controller the opportunity to implement the behavior
// in response to this event, intercepting the event and superseding the Pawn's default
// behavior.
//
// The control rotation (accessed via GetControlRotation()), determines the aiming
// orientation of the controlled Pawn.
//
//=============================================================================

#pragma once
#include "Controller.generated.h"

class UNavigationComponent;
class UPathFollowingComponent;

UDELEGATE(BlueprintAuthorityOnly)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams( FInstigatedAnyDamageSignature, float, Damage, const class UDamageType*, DamageType, class AActor*, DamagedActor, class AActor*, DamageCauser );

UCLASS(abstract, notplaceable, NotBlueprintable, HideCategories=(Collision,Rendering,"Utilities|Orientation")) 
class ENGINE_API AController : public AActor, public INavAgentInterface
{
	GENERATED_UCLASS_BODY()

private:
	/** Pawn currently being controlled by this controller.  Use Pawn.Possess() to take control of a pawn */
	UPROPERTY(replicatedUsing=OnRep_Pawn)
	class APawn* Pawn;

	/**
	 * Used to track when pawn changes during OnRep_Pawn. 
	 * It's possible to use a OnRep parameter here, but I'm not sure what happens with pointers yet so playing it safe.
	 */
	TWeakObjectPtr< class APawn > OldPawn;

	/** Character currently being controlled by this controller.  Value is same as Pawn if the controlled pawn is a character, otherwise NULL */
	UPROPERTY()
	class ACharacter* Character;

public:
	/** PlayerState containing replicated information about the player using this controller (only exists for players, not NPCs). */
	UPROPERTY(editinline, replicatedUsing=OnRep_PlayerState)
	class APlayerState* PlayerState;

protected:
	/** Component to give controllers a transform and enable attachment if desired. */
	UPROPERTY()
	TSubobjectPtr<class USceneComponent> TransformComponent;

public:

	/**
	  * Get the control rotation. This is the full aim rotation, which may be different than a camera orientation (for example in a third person view),
	  * and may differ from the rotation of the controlled Pawn (which may choose not to visually pitch or roll, for example).
	  */
	UFUNCTION(BlueprintCallable, Category="Pawn")
	virtual FRotator GetControlRotation() const;

	/** Set the control rotation. The RootComponent's rotation will also be updated to match it if RootComponent->bAbsoluteRotation is true. */
	UFUNCTION(BlueprintCallable, Category="Pawn", meta=(Tooltip="Set the control rotation."))
	virtual void SetControlRotation(const FRotator& NewRotation);

	/** Set the initial location and rotation of the controller, as well as the control rotation. Typically used when the controller is first created. */
	UFUNCTION(BlueprintCallable, Category="Pawn")
	virtual void SetInitialLocationAndRotation(const FVector& NewLocation, const FRotator& NewRotation);

protected:

	/** The control rotation of the Controller. See GetControlRotation. */
	UPROPERTY()
	FRotator ControlRotation;

	/**
	 * If true, the controller location will match the possessed Pawn's location. If false, it will not be updated. Rotation will match ControlRotation in either case.
	 * Since a Controller's location is normally inaccessible, this is intended mainly for purposes of being able to attach
	 * an Actor that follows the possessed Pawn location, but that still has the full aim rotation (since a Pawn might
	 * update only some components of the rotation).
	 */
	UPROPERTY(EditDefaultsOnly, AdvancedDisplay, Category="Controller|Transform")
	uint32 bAttachToPawn:1;

	/**
	  * Physically attach the Controller to the specified Pawn, so that our position reflects theirs.
	  * The attachment persists during possession of the pawn. The Controller's rotation continues to match the ControlRotation.
	  * Attempting to attach to a NULL Pawn will call DetachFromPawn() instead.
	  */
	virtual void AttachToPawn(APawn* InPawn);

	/** Detach the RootComponent from its parent, but only if bAttachToPawn is true and it was attached to a Pawn.	 */
	virtual void DetachFromPawn();

	/** Add dependency that makes us tick before the given Pawn. This minimizes latency between input processing and Pawn movement.	 */
	virtual void AddPawnTickDependency(APawn* NewPawn);

	/** Remove dependency that makes us tick before the given Pawn.	 */
	virtual void RemovePawnTickDependency(APawn* OldPawn);

public:

	/** Actor marking where this controller spawned in. */
	TWeakObjectPtr<class AActor> StartSpot;

	//=============================================================================
	// CONTROLLER STATE PROPERTIES

	UPROPERTY()
	FName StateName;

	/** Change the current state to named state */
	virtual void ChangeState(FName NewState);

	/** 
	 * States (uses FNames for replication, correlated to state flags) 
	 * @param StateName the name of the state to test against
	 * @return true if current state is StateName
	 */
	bool IsInState(FName InStateName);
	
	/** @return the name of the current state */
	FName GetStateName() const;
	
	/**
	 * Checks line to center and top of other actor
	 * @param Other is the actor whose visibility is being checked.
	 * @param ViewPoint is eye position visibility is being checked from.  If vect(0,0,0) passed in, uses current viewtarget's eye position.
	 * @param bAlternateChecks used only in AIController implementation
	 * @return true if controller's pawn can see Other actor.
	 */
	virtual bool LineOfSightTo(const class AActor* Other, FVector ViewPoint = FVector(ForceInit), bool bAlternateChecks = false);

	/** Replication Notification Callbacks */
	UFUNCTION()
	virtual void OnRep_Pawn();

	UFUNCTION()
	virtual void OnRep_PlayerState();

	/** @fixme, shouldn't be necessary once general cast support is in. */
	UFUNCTION(BlueprintCallable, Category="Pawn")
	class APlayerController* CastToPlayerController();

	/** Replicated function to set the pawn location and rotation, allowing server to force (ex. teleports). */
	UFUNCTION(reliable, client)
	void ClientSetLocation(FVector NewLocation, FRotator NewRotation);

	/** Replicated function to set the pawn rotation, allowing the server to force. */
	UFUNCTION(reliable, client)
	void ClientSetRotation(FRotator NewRotation, bool bResetCamera = false);

	/** Return the Pawn that is currently 'controlled' by this PlayerController */
	UFUNCTION(BlueprintCallable, Category="Pawn")
	class APawn* GetControlledPawn() const;

	// Begin AActor interface
	virtual void TickActor( float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction ) OVERRIDE;
	virtual void K2_DestroyActor() OVERRIDE;
	virtual void DisplayDebug(class UCanvas* Canvas, const TArray<FName>& DebugDisplay, float& YL, float& YPos) OVERRIDE;
	virtual void GetActorEyesViewPoint( FVector& out_Location, FRotator& out_Rotation ) const OVERRIDE;
	virtual FString GetHumanReadableName() const OVERRIDE;

	/* Overridden to create the player replication info and perform other mundane initialization tasks. */
	virtual void PostInitializeComponents() OVERRIDE;

	virtual void Reset() OVERRIDE;
	virtual void Destroyed() OVERRIDE;
	// End AActor interface

	/** Getter for Pawn */
	FORCEINLINE APawn* GetPawn() const { return Pawn; }

	/** Getter for Character */
	FORCEINLINE ACharacter* GetCharacter() const { return Character; }

	/** Setter for Pawn. Normally should only be used internally when possessing/unpossessing a Pawn. */
	virtual void SetPawn(APawn* InPawn);

	/** Calls SetPawn and RepNotify locally */
	void SetPawnFromRep(APawn* InPawn);

	/** Get the actor the controller is looking at */
	UFUNCTION(BlueprintCallable, Category="Pawn")
	virtual AActor* GetViewTarget() const;

	/** Get the desired pawn target rotation */
	UFUNCTION(BlueprintCallable, Category="Pawn")
	virtual FRotator GetDesiredRotation() const;

	/** returns whether this Controller is a locally controlled PlayerController.  */
	UFUNCTION(BlueprintCallable, Category="Pawn")
	virtual bool IsLocalPlayerController() const;

	/** Returns whether this Controller is a local controller.	 */
	UFUNCTION(BlueprintCallable, Category="Pawn")
	virtual bool IsLocalController() const;

	/** Called from Destroyed().  Cleans up PlayerState. */
	virtual void CleanupPlayerState();

	/** Handles attaching this controller to the specified pawn.
	 * @param inPawn is the pawn to be possessed
	 */
	UFUNCTION(BlueprintCallable, Category="Pawn")
	virtual void Possess(class APawn* inPawn);

	/**
	 * Called to unpossess our pawn for any reason that is not the pawn being destroyed
	 * (destruction handled by PawnDestroyed())
	 */
	UFUNCTION(BlueprintCallable, Category="Pawn")
	virtual void UnPossess();

	/**
	 * Called to unpossess our pawn because it is going to be destroyed.
	 * (other unpossession handled by UnPossess())
	 */
	virtual void PawnPendingDestroy(class APawn* inPawn);

	/**
	 * Notification of someone in the game being killed (to be overridden for things classes like AI)
	 * @param	AController * Killer - The controller of the player who did the killing
	 * @param	AController * KilledPlayer - The controller of the player who got killed
	 * @param	APawn * KilledPawn - The pawn of the player who got killed
	 * @param	const UDamageType * DamageType - The damage type used to kill the player
	 */
	virtual void NotifyKilled(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, const UDamageType* DamageType) {}

	/** Called when this controller instigates ANY damage */
	virtual void InstigatedAnyDamage(float Damage, const class UDamageType* DamageType, class AActor* DamagedActor, class AActor* DamageCauser);

	/** spawns and initializes the PlayerState for this Controller */
	virtual void InitPlayerState();

	/**
	 * Called from game mode upon end of the game, used to transition to proper state. 
	 * @param EndGameFocus Actor to set as the view target on end game
	 * @param bIsWinner true if this controller is on winning team
	 */
	virtual void GameHasEnded(class AActor* EndGameFocus = NULL, bool bIsWinner = false);

	/**
	 * Returns Player's Point of View
	 * For the AI this means the Pawn's 'Eyes' ViewPoint
	 * For a Human player, this means the Camera's ViewPoint
	 *
	 * @output	out_Location, view location of player
	 * @output	out_rotation, view rotation of player
	*/
	virtual void GetPlayerViewPoint( FVector& Location, FRotator& Rotation ) const;

	/** GameMode failed to spawn pawn for me. */
	virtual void FailedToSpawnPawn();

	// Begin INavAgentInterface Interface
	virtual const struct FNavAgentProperties* GetNavAgentProperties() const OVERRIDE;
	virtual FVector GetNavAgentLocation() const OVERRIDE;
	virtual void GetMoveGoalReachTest(class AActor* MovingActor, const FVector& MoveOffset, FVector& GoalOffset, float& GoalRadius, float& GoalHalfHeight) const OVERRIDE;
	// End INavAgentInterface Interface

	/** prepares path finding and path following components */
	virtual void InitNavigationControl(UNavigationComponent*& PathFindingComp, UPathFollowingComponent*& PathFollowingComp);

	/** If controller has any navigation-related components then this function 
	 *	makes them update their cached data */
	virtual void UpdateNavigationComponents();

protected:
	/** State entered when inactive (no possessed pawn, not spectating, etc). */
	virtual void BeginInactiveState();

	/** State entered when inactive (no possessed pawn, not spectating, etc). */
	virtual void EndInactiveState();

public:
	/** Called when the level this controller is in is unloaded via streaming. */
	virtual void CurrentLevelUnloaded();

private:
	// Hidden functions that don't make sense to use on this class.
	HIDE_ACTOR_TRANSFORM_FUNCTIONS();

	/** Event when this controller instigates ANY damage */
	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly)
	virtual void ReceiveInstigatedAnyDamage(float Damage, const class UDamageType* DamageType, class AActor* DamagedActor, class AActor* DamageCauser);

	/** Called when the controller has instigated damage in any way */
	UPROPERTY(BlueprintAssignable)
	FInstigatedAnyDamageSignature OnInstigatedAnyDamage;
};
