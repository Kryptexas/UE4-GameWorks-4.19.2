// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Controller.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Character.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMovementModeChangedSignature, class ACharacter*, Character, EMovementMode, PrevMovementMode, uint8, PreviousCustomMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCharacterMovementUpdatedSignature, float, DeltaSeconds, FVector, OldLocation, FVector, OldVelocity);
//
// Forward declarations
//
struct FAnimMontageInstance;
/** Replicated data when playing a root motion montage. */
USTRUCT()
struct FRepRootMotionMontage
{
	GENERATED_USTRUCT_BODY()

	/** AnimMontage providing Root Motion */
	UPROPERTY()
	class UAnimMontage* AnimMontage;

	/** Track position of Montage */
	UPROPERTY()
	float Position;

	/** Location */
	UPROPERTY()
	FVector_NetQuantize10 Location;

	/** Rotation */
	UPROPERTY()
	FRotator Rotation;

	/** Movement Relative to Base */
	UPROPERTY()
	UPrimitiveComponent* MovementBase;

	/** Additional replicated flag, if MovementBase can't be resolved on the client. So we don't use wrong data. */
	UPROPERTY()
	bool bRelativePosition;

	/** Clear the montage */
	void Clear()
	{
		AnimMontage = NULL;
	}

	/** Is Valid */
	bool HasRootMotion() const
	{
		return (AnimMontage != NULL);
	}
};

USTRUCT()
struct FSimulatedRootMotionReplicatedMove
{
	GENERATED_USTRUCT_BODY()

	/** Local time when move was received on client and saved. */
	UPROPERTY()
	float Time;

	/** Root Motion information */
	UPROPERTY()
	FRepRootMotionMontage RootMotion;
};


/** Utility for determining if we should use relative positioning when based on a component (because it may move). */
namespace MovementBaseUtility
{
	FORCEINLINE bool IsDynamicBase(const class UPrimitiveComponent* MovementBase)
	{
		return (MovementBase && MovementBase->Mobility == EComponentMobility::Movable);
	}

	FORCEINLINE bool UseRelativePosition(const class UPrimitiveComponent* MovementBase)
	{
		return IsDynamicBase(MovementBase);
	}

	/** Ensure that BasedObjectTick ticks after NewBase */
	void AddTickDependency(FTickFunction& BasedObjectTick, class UPrimitiveComponent* NewBase);

	/** Remove tick dependency of BasedObjectTick on OldBase */
	void RemoveTickDependency(FTickFunction& BasedObjectTick, class UPrimitiveComponent* OldBase);

	/** Get the velocity of the given component, first checking the ComponentVelocity and falling back to the physics velocity if necessary. */
	FVector GetMovementBaseVelocity(const class UPrimitiveComponent* MovementBase);

	/** Get the tangential velocity at WorldLocation for the given component. */
	FVector GetMovementBaseTangentialVelocity(const class UPrimitiveComponent* MovementBase, const FVector& WorldLocation);
}

/** Struct to hold relative position information from the server. */
USTRUCT()
struct FRepRelativeMovement
{
	GENERATED_USTRUCT_BODY()

	/** Component we are based on */
	UPROPERTY()
	class UPrimitiveComponent* MovementBase;

	/** Location relative to MovementBase. Only valid if HasRelativePosition() is true. */
	UPROPERTY()
	FVector_NetQuantize100 Location;

	/** Rotation relative to MovementBase. Only valid if HasRelativePosition() is true. */
	UPROPERTY()
	FRotator Rotation;

	/** Whether the server says that there is a base. On clients, the component may not have resolved yet. */
	UPROPERTY()
	bool bServerHasBaseComponent;


	/** Is position relative? */
	FORCEINLINE bool HasRelativePosition() const
	{
		return MovementBaseUtility::UseRelativePosition(MovementBase);
	}

	/** Return true if the client should have MovementBase, but it hasn't replicated (possibly component has not streamed in). */
	FORCEINLINE bool IsBaseUnresolved() const
	{
		return (MovementBase == NULL) && bServerHasBaseComponent;
	}
};

//=============================================================================
// Characters are Pawns that have a mesh, collision, and physics and are responsible for all
// physical interaction between the player or AI and the world. They are strongly tied
// to a CharacterMovementComponent that handles movement of the collision capsule, and they
// also have implementations of basic networking and input models.
//=============================================================================

UCLASS(abstract, config=Game, BlueprintType, hidecategories=("Pawn|Character|InternalEvents"))
class ENGINE_API ACharacter : public APawn
{
	GENERATED_UCLASS_BODY()

	/** The main skeletal mesh associated with this Character (optional sub-object). */
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly)
	TSubobjectPtr<class USkeletalMeshComponent> Mesh;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TSubobjectPtr<class UArrowComponent> ArrowComponent;
#endif

	/** CharacterMovement component used by walking/running/flying avatars not using rigid body physics */
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly)
	TSubobjectPtr<class UCharacterMovementComponent> CharacterMovement;

	/** The CapsuleComponent being used for movement collision (if has CharacterMovement). Always treated as being vertically aligned in simple collision check functions. */
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly)
	TSubobjectPtr<class UCapsuleComponent> CapsuleComponent;

	/** Name of the MeshComponent. Use this name if you want to prevent creation of the component (with PCIP.DoNotCreateDefaultSubobject). */
	static FName MeshComponentName;

	/** Name of the CharacterMovement. Use this name if you want to use a different class (with PCIP.SetDefaultSubobjectClass). */
	static FName CharacterMovementComponentName;

	/** Name of the CapsuleComponent. */
	static FName CapsuleComponentName;

	/** Sets the MovementBase used by CharacterMovement walking movement. */
	virtual void SetBase(UPrimitiveComponent* NewBase, bool bNotifyActor=true);

protected:
	// CharacterMovement basing related variables (here for replication purposes).
	//

	/** Component we are based on */
	UPROPERTY()
	class UPrimitiveComponent* MovementBase;

	/** Desired translation offset of mesh. */
	UPROPERTY()
	FVector BaseTranslationOffset;

	/** Event called after actor's base changes (if SetBase was requested to notify us with bNotifyPawn). */
	virtual void BaseChange();

	// Always called immediately after properties are received from the remote.
	virtual void PostNetReceiveBase();

	/** CharacterMovement MovementMode (and custom mode) replicated for simulated proxies. Use CharacterMovementComponent::UnpackNetworkMovementMode() to translate it. */
	UPROPERTY(Replicated)
	uint8 ReplicatedMovementMode;

public:	

	/** Returns ReplicatedMovementMode */
	uint8 GetReplicatedMovementMode() const { return ReplicatedMovementMode; }

	/** @return Desired translation offset of mesh. */
	const FVector& GetBaseTranslationOffset() const { return BaseTranslationOffset; }

	/** location, rotation and velocity relative to base/bone (valid if base exists) */
	UPROPERTY(replicated)
	struct FRepRelativeMovement RelativeMovement;

	/** Default crouched eye height */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Camera)
	float CrouchedEyeHeight;

	/** Set by character movement to specify that this Character is currently crouched. */
	UPROPERTY(BlueprintReadOnly, replicatedUsing=OnRep_IsCrouched, Category=Character)
	uint32 bIsCrouched:1;

	/** Handle Crouching replicated from server */
	UFUNCTION()
	virtual void OnRep_IsCrouched();

	/** When true, player wants to jump */
	UPROPERTY(BlueprintReadOnly, Category="Pawn|Character")
	uint32 bPressedJump:1;

	/** When true, applying updates to network client (replaying saved moves for a locally controlled character) */
	UPROPERTY(Transient)
	uint32 bClientUpdating:1;

	/** True if Pawn was initially falling when started to replay network moves. */
	UPROPERTY(Transient)
	uint32 bClientWasFalling:1; 

	/** If server disagrees with root motion track position, client has to resimulate root motion from last AckedMove. */
	UPROPERTY(Transient)
	uint32 bClientResimulateRootMotion:1;

	/** Disable simulated gravity (set when character encroaches geometry on client, to keep him from falling through floors) */
	UPROPERTY()
	uint32 bSimGravityDisabled:1;

	/** 
	 * Jump key Held Time.
	 * This is the time that the player has held the jump key, in seconds.
	 */
	UPROPERTY(Transient, BlueprintReadOnly, VisibleInstanceOnly, Category=Character)
	float JumpKeyHoldTime;

	/** 
	 * The max time the jump key can be held.
	 * Note that if StopJumping() is not called before the max jump hold time is reached,
	 * then the character will carry on receiving vertical velocity. Therefore it is usually 
	 * best to call StopJumping() when jump input has ceased (such as a button up event).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category=Character)
	float JumpMaxHoldTime;

	// Begin AActor Interface.
	virtual void TeleportSucceeded(bool bIsATest) override;
	virtual void ClearCrossLevelReferences() override;
	virtual void PreNetReceive() override;
	virtual void PostNetReceive() override;
	virtual void OnRep_ReplicatedMovement() override;
	virtual void PostNetReceiveLocation() override;
	virtual void GetSimpleCollisionCylinder(float& CollisionRadius, float& CollisionHalfHeight) const override;
	virtual void ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) override;
	virtual UActorComponent* FindComponentByClass(const TSubclassOf<UActorComponent> ComponentClass) const override;
	virtual void TornOff() override;
	// End AActor Interface

	template<class T>
	T* FindComponentByClass() const
	{
		return AActor::FindComponentByClass<T>();
	}

	// Begin APawn Interface.
	virtual void PostInitializeComponents() override;
	virtual class UPawnMovementComponent* GetMovementComponent() const override { return CharacterMovement; }
	virtual class UPrimitiveComponent* GetMovementBase() const override final { return MovementBase; }
	virtual float GetDefaultHalfHeight() const override;
	virtual void TurnOff() override;
	virtual void Restart() override;
	virtual void PawnClientRestart() override;
	virtual void UnPossessed() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	virtual void DisplayDebug(class UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;
	virtual void RecalculateBaseEyeHeight() override;
	// End APawn Interface

	/** Apply momentum caused by damage. */
	virtual void ApplyDamageMomentum(float DamageTaken, FDamageEvent const& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser);

	/** 
	 * Make the character jump on the next update.	 
	 * If you want your character to jump according to the time that the jump key is held,
	 * then you can set JumpKeyHoldTime to some non-zero value. Make sure in this case to
	 * call StopJumping() when you want the jump's z-velocity to stop being applied (such 
	 * as on a button up event), otherwise the character will carry on receiving the 
	 * velocity until JumpKeyHoldTime is reached.
	 */
	UFUNCTION(BlueprintCallable, Category="Pawn|Character")
	virtual void Jump();

	/** 
	 * Stop the character from jumping on the next update. 
	 * Call this from an input event (such as a button 'up' event) to cease applying
	 * jump Z-velocity. If this is not called, then jump z-velocity will be applied
	 * until JumpMaxHoldTime is reached.
	 */
	UFUNCTION(BlueprintCallable, Category="Pawn|Character")
	virtual void StopJumping();

	/**
	 * Check if the character can jump in the current state.
	 *
	 * The default implementation may be overridden or extended by implementing the custom CanJump event.
	 * 
	 * @Return Whether the character can jump in the current state. 
	 */
	UFUNCTION(BlueprintCallable, Category="Pawn|Character")
	bool CanJump() const;

protected:

	/**
	 * Customizable event to check if the character can jump in the current state.
	 * Default implementation returns true if the character is on the ground and not crouching,
	 * has a valid CharacterMovementComponent and CanEverJump() returns true.
	 * Default implementation also allows for 'hold to jump higher' functionality: 
	 * As well as returning true when on the ground, it also returns true when GetMaxJumpTime is more
	 * than zero and IsJumping returns true.
	 * 
	 * @Return Whether the character can jump in the current state. 
	 */

	UFUNCTION(BlueprintNativeEvent, Category="Pawn|Character|InternalEvents", meta=(FriendlyName="CanJump"))
	bool CanJumpInternal() const;

public:

	/** True if currently jumping; i.e. jump key is held and the time it has been held is less than JumpMaxHoldTime */
	UFUNCTION(BlueprintCallable, Category="Pawn|Character")
	virtual bool IsJumping() const;

	/** Play Animation Montage on the character mesh **/
	UFUNCTION(BlueprintCallable, Category=Animation)
	virtual float PlayAnimMontage(class UAnimMontage* AnimMontage, float InPlayRate = 1.f, FName StartSectionName = NAME_None);

	/** Stop Animation Montage. If NULL, it will stop what's currently active **/
	UFUNCTION(BlueprintCallable, Category=Animation)
	virtual void StopAnimMontage(class UAnimMontage* AnimMontage = NULL);

	/** Return current playing Montage **/
	UFUNCTION(BlueprintCallable, Category=Animation)
	class UAnimMontage * GetCurrentMontage();

protected:

	/**
	 * Player Jumped. Called internally when a jump has been detected because bPressedJump was true.
	 * @param	bReplayingMoves: true if this is being done as part of replaying moves on a locally controlled client after a server correction.
	 * @return	True if the jump was allowed by CanJump() and if CharacterMovement->Jump() succeeded.
	 **/ 
	virtual bool DoJump(bool bReplayingMoves);

public:

	/** Set a pending launch velocity on the Character. This velocity will be processed on the next CharacterMovementComponent tick,
	  * and will set it to the "falling" state. Triggers the OnLaunched event.
	  * @PARAM LaunchVelocity is the velocity to impart to the Character
	  * @PARAM bXYOverride if true replace the XY part of the Character's velocity instead of adding to it.
	  * @PARAM bZOverride if true replace the Z component of the Character's velocity instead of adding to it.
	  */
	UFUNCTION(BlueprintCallable, Category="Pawn|Character")
	virtual void LaunchCharacter(FVector LaunchVelocity, bool bXYOverride, bool bZOverride);

	/** Let blueprint know that we were launched */
	UFUNCTION(BlueprintImplementableEvent)
	virtual void OnLaunched(FVector LaunchVelocity, bool bXYOverride, bool bZOverride);

	/** Event fired when the character has just started jumping */
	UFUNCTION(BlueprintNativeEvent, Category="Pawn|Character")
	void OnJumped();

	/** Called when the character's movement enters falling */
	virtual void Falling() {}

	/** Called when character's jump reaches Apex. Needs CharacterMovement->bNotifyApex = true */
	virtual void NotifyJumpApex() {}

	/** Called on landing after falling has completed, to perform actions based on the Hit result. Triggers the OnLanded event. */
	virtual void Landed(const FHitResult& Hit);

	/** Called on landing after falling has completed, to perform actions based on the Hit result. */
	UFUNCTION(BlueprintImplementableEvent)
	virtual void OnLanded(const FHitResult& Hit);

	/**
	 * Event fired when the Character is walking off a surface and is about to fall because CharacterMovement->CurrentFloor became unwalkable.
	 * If CharacterMovement->MovementMode does not change (from Walking) during this event then the character will start falling.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Pawn|Character")
	void OnWalkingOffLedge();

	/** Called when pawn's movement is blocked
		@PARAM Impact describes the blocking hit. */
	virtual void MoveBlockedBy(const FHitResult& Impact) {};

	/** Make the character start crouching.	 */
	UFUNCTION(BlueprintCallable, Category="Pawn|Character", meta=(HidePin="bClientSimulation"))
	virtual void Crouch(bool bClientSimulation = false);

	/** Make the character stop crouching.	 */
	UFUNCTION(BlueprintCallable, Category="Pawn|Character", meta=(HidePin="bClientSimulation"))
	virtual void UnCrouch(bool bClientSimulation = false);

	/** @return true if this character is currently able to crouch (and is not currently crouched) */
	virtual bool CanCrouch();

	/** 
	 * Called when Character stops crouching. Called on non-owned Characters through bIsCrouched replication.
	 * @param	HalfHeightAdjust		difference between default collision half-height, and actual crouched capsule half-height.
	 * @param	ScaledHalfHeightAdjust	difference after component scale is taken in to account.
	 */
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust);

	/** 
	 * Event when Character stops crouching.
	 * @param	HalfHeightAdjust		difference between default collision half-height, and actual crouched capsule half-height.
	 * @param	ScaledHalfHeightAdjust	difference after component scale is taken in to account.
	 */
	UFUNCTION(BlueprintImplementableEvent, meta=(FriendlyName = "OnEndCrouch"))
	virtual void K2_OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust);

	/**
	 * Called when Character crouches. Called on non-owned Characters through bIsCrouched replication.
	 * @param	HalfHeightAdjust		difference between default collision half-height, and actual crouched capsule half-height.
	 * @param	ScaledHalfHeightAdjust	difference after component scale is taken in to account.
	 */
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust);

	/**
	 * Event when Character crouches.
	 * @param	HalfHeightAdjust		difference between default collision half-height, and actual crouched capsule half-height.
	 * @param	ScaledHalfHeightAdjust	difference after component scale is taken in to account.
	 */
	UFUNCTION(BlueprintImplementableEvent, meta=(FriendlyName = "OnStartCrouch"))
	virtual void K2_OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust);

	/**
	 * Called from CharacterMovementComponent to notify the character that the movement mode has changed.
	 * @param	PrevMovementMode	Movement mode before the change
	 * @param	PrevCustomMode		Custom mode before the change (applicable if PrevMovementMode is Custom)
	 */
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0);

	/** Native multicast delegate for MovementMode changing. */
	FMovementModeChangedSignature MovementModeChangedDelegate;
	/**
	 * Called from CharacterMovementComponent to notify the character that the movement mode has changed.
	 * @param	PrevMovementMode	Movement mode before the change
	 * @param	NewMovementMode		New movement mode
	 * @param	PrevCustomMode		Custom mode before the change (applicable if PrevMovementMode is Custom)
	 * @param	NewCustomMode		New custom mode (applicable if NewMovementMode is Custom)
	 */
	UFUNCTION(BlueprintImplementableEvent, meta=(FriendlyName = "OnMovementModeChanged"))
	virtual void K2_OnMovementModeChanged(EMovementMode PrevMovementMode, EMovementMode NewMovementMode, uint8 PrevCustomMode, uint8 NewCustomMode);

	/** Event for implementing custom character movement mode. Called by CharacterMovement if MovementMode is set to Custom. */
	UFUNCTION(BlueprintImplementableEvent, meta=(FriendlyName= "UpdateCustomMovement"))
	virtual void K2_UpdateCustomMovement(float DeltaTime);

	/**
	 * Event triggered at the end of a CharacterMovementComponent movement update.
	 * This is the preferred event to use rather than the Tick event when performing custom updates to CharacterMovement properties based on the current state.
	 * This is mainly due to the nature of network updates, where client corrections in position from the server can cause multiple iterations of a movement update,
	 * which allows this event to update as well, while a Tick event would not.
	 *
	 * @param	DeltaSeconds		Delta time in seconds for this update
	 * @param	InitialLocation		Location at the start of the update. May be different than the current location if movement occurred.
	 * @param	InitialVelocity		Velocity at the start of the update. May be different than the current velocity.
	 */
	UPROPERTY(BlueprintAssignable, Category="Pawn|Character")
	FCharacterMovementUpdatedSignature OnCharacterMovementUpdated;

	/**
	 * Called when our pawn has landed from a fall.
	 * @param	PrevMovementMode	Movement mode before the change
	 */
	virtual bool NotifyLanded(const struct FHitResult& Hit);

	/** Trigger jump if jump button has been pressed. */
	virtual void CheckJumpInput(float DeltaTime);

	/** Reset jump input state after having checked input. */
	virtual void ClearJumpInput();

	/**
	 * Get the maximum jump time for the character.
	 * Note that if StopJumping() is not called before the max jump hold time is reached,
	 * then the character will carry on receiving vertical velocity. Therefore it is usually 
	 * best to call StopJumping() when jump input has ceased (such as a button up event).
	 * 
	 * @return Maximum jump time for the character
	 */
	virtual float GetJumpMaxHoldTime() const;

	DEPRECATED(4.4, "UpdateFromCompressedFlags has moved to UCharacterMovementComponent")
	virtual void UpdateFromCompressedFlags(uint8 Flags) { }

public:
	// Note: these network functions should be moved to the Character movement component. They are currently just wrappers that pass the calls to CharacterMovement.

	/** Replicated function sent by client to server - contains client movement and view info. */
	UFUNCTION(unreliable, server, WithValidation)
	void ServerMove(float TimeStamp, FVector_NetQuantize100 InAccel, FVector_NetQuantize100 ClientLoc, uint8 MoveFlags, uint8 ClientRoll, uint32 View, class UPrimitiveComponent* ClientMovementBase, uint8 ClientMovementMode);

	/** Replicated function sent by client to server - contains client movement and view info for two moves. */
	UFUNCTION(unreliable, server, WithValidation)
	void ServerMoveDual(float TimeStamp0, FVector_NetQuantize100 InAccel0, uint8 PendingFlags, uint32 View0, float TimeStamp, FVector_NetQuantize100 InAccel, FVector_NetQuantize100 ClientLoc, uint8 NewFlags, uint8 ClientRoll, uint32 View, class UPrimitiveComponent* ClientMovementBase, uint8 ClientMovementMode);

	/* Resending an (important) old move.  Process it if not already processed. */
	UFUNCTION(unreliable, server, WithValidation)
	void ServerMoveOld(float OldTimeStamp, FVector_NetQuantize100 OldAccel, uint8 OldMoveFlags);

	/** If no client adjustment is needed after processing received ServerMove(), ack the good move so client can remove it from SavedMoves */
	UFUNCTION(unreliable, client)
	void ClientAckGoodMove(float TimeStamp);

	/** Replicate position correction to client, associated with a timestamped servermove.  Client will replay subsequent moves after applying adjustment.  */
	UFUNCTION(unreliable, client)
	void ClientAdjustPosition(float TimeStamp, FVector NewLoc, FVector NewVel, class UPrimitiveComponent* NewBase, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode);

	/* Bandwidth saving version, when velocity is zeroed */
	UFUNCTION(unreliable, client)
	void ClientVeryShortAdjustPosition(float TimeStamp, FVector NewLoc, class UPrimitiveComponent* NewBase, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode);

	/** Replicate position correction to client when using root motion for mevement. */
	UFUNCTION(unreliable, client)
	void ClientAdjustRootMotionPosition(float TimeStamp, float ServerMontageTrackPosition, FVector ServerLoc, FVector_NetQuantizeNormal ServerRotation, float ServerVelZ, class UPrimitiveComponent* ServerBase, bool bHasBase, bool bBaseRelativePosition, uint8 ServerMovementMode);

	UFUNCTION(reliable, client)
	void ClientCheatWalk();

	UFUNCTION(reliable, client)
	void ClientCheatFly();

	UFUNCTION(reliable, client)
	void ClientCheatGhost();

	// Root Motion
public:
	/** For LocallyControlled Autonomous clients. Saved root motion data to be used by SavedMoves. */
	UPROPERTY(Transient)
	FRootMotionMovementParams ClientRootMotionParams;

	/** Array of previously received root motion moves from the server. */
	UPROPERTY(Transient)
	TArray<FSimulatedRootMotionReplicatedMove> RootMotionRepMoves;
	
	/** Find usable root motion replicated move from our buffer.
	 * Goes through the buffer back in time, to find the first move that clears 'CanUseRootMotionRepMove' below.
	 * Returns index of that move or INDEX_NONE otherwise. */
	int32 FindRootMotionRepMove(const FAnimMontageInstance & ClientMontageInstance) const;

	/** true if buffered move is usable to teleport client back to. */
	bool CanUseRootMotionRepMove(const FSimulatedRootMotionReplicatedMove & RootMotionRepMove, const FAnimMontageInstance & ClientMontageInstance) const;

	/** Restore actor to an old buffered move. */
	bool RestoreReplicatedMove(const FSimulatedRootMotionReplicatedMove & RootMotionRepMove);
	
	/** Called on client after position update is received to actually move the character. */
	void UpdateSimulatedPosition(const FVector & NewLocation, const FRotator & NewRotation);

	/** Replicated Root Motion montage */
	UPROPERTY(ReplicatedUsing=OnRep_RootMotion)
	struct FRepRootMotionMontage RepRootMotion;
	
	/** Handles replicated root motion properties on simulated proxies and position correction. */
	UFUNCTION()
	void OnRep_RootMotion();

	/** Position fix up for Simulated Proxies playing Root Motion */
	void SimulatedRootMotionPositionFixup(float DeltaSeconds);

	/** Get FAnimMontageInstance playing RootMotion */
	FAnimMontageInstance * GetRootMotionAnimMontageInstance() const;

	/** true if we are playing Root Motion right now */
	UFUNCTION(BlueprintCallable, Category=Animation)
	bool IsPlayingRootMotion() const;

	/** Called on the actor right before replication occurs */
	virtual void PreReplication( IRepChangedPropertyTracker & ChangedPropertyTracker ) override;
};
