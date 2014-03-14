// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Pawn.generated.h"

ENGINE_API DECLARE_LOG_CATEGORY_EXTERN(LogDamage, Warning, All);

/** 
 *	Pawn is the base class of all actors that can be possessed by players or AI.
 *	Pawns are the physical representations of players and creatures in a level.
 *	They are responsible for all physical interaction between the player or AI and the world.
 */
UCLASS(abstract, HeaderGroup=Pawn, config=Game, dependson=AController, BlueprintType, Blueprintable, hidecategories=(Navigation, "AI|Navigation"))
class ENGINE_API APawn : public AActor, public INavAgentInterface
{
	GENERATED_UCLASS_BODY()

	/** Return our PawnMovementComponent, if we have one. By default, returns the first PawnMovementComponent found. Native classes that create their own movement component should override this method for more efficiency. */
	UFUNCTION(BlueprintCallable, meta=(Tooltip="Return our PawnMovementComponent, if we have one."), Category="Pawn")
	virtual class UPawnMovementComponent* GetMovementComponent() const;

	/** Return Actor we are based on (standing on, attached to, and moving on). */
	virtual class UPrimitiveComponent* GetMovementBase() const { return NULL; }

public:
	/** If true, this Pawn's pitch will be updated to match the Controller's ControlRotation pitch, if controlled by a PlayerController. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Pawn)
	uint32 bUseControllerRotationPitch:1;

	/** If true, this Pawn's yaw will be updated to match the Controller's ControlRotation yaw, if controlled by a PlayerController. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Pawn)
	uint32 bUseControllerRotationYaw:1;

	/** If true, this Pawn's roll will be updated to match the Controller's ControlRotation roll, if controlled by a PlayerController. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Pawn)
	uint32 bUseControllerRotationRoll:1;

private:
	/* Whether this Pawn's input handling is enabled.  Pawn must still be possessed to get input even if this is true */
	uint32 bInputEnabled:1;

	/** Used for debugging gameplay across network connections.  Not present in shipping builds. */
	UPROPERTY(Replicated, Transient)
	class UGameplayDebuggingComponent* DebugComponent;

public:

	/** Base eye height above collision center. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Camera)
	float BaseEyeHeight;

	/* Specifies which player controller, if any, should automatically possess the pawn when the level starts or the pawn is spawned */
	UPROPERTY(EditAnywhere, Category=Pawn)
	TEnumAsByte<EAutoReceiveInput::Type> AutoPossess;

	/** Class of GameplayDebuggingComponent to instantiate. */
	UClass* GameplayDebuggingComponentClass;

	virtual class UGameplayDebuggingComponent* GetDebugComponent(bool bCreateIfNotFound=false);
	virtual void RemoveDebugComponent();

public:

	/**
	 * Return our PawnNoiseEmitterComponent, if any. Default implementation returns the first PawnNoiseEmitterComponent found in the components array.
	 * If one isn't found, then it tries to find one on the Pawn's current Controller.
	 */
	virtual class UPawnNoiseEmitterComponent* GetPawnNoiseEmitterComponent() const;

	/**
	 * Inform AIControllers that you've made a noise they might hear (they are sent a HearNoise message if they have bHearNoises==true)
	 * The instigator of this sound is the pawn which is used to call MakeNoise.
	 *
	 * @param Loudness - is the relative loudness of this noise (range 0.0 to 1.0).  Directly affects the hearing range specified by the AI's HearingThreshold.
	 * @param NoiseLocation - Position of noise source.  If zero vector, use the actor's location.
	 * @param bUseNoiseMakerLocation - If true, use the location of the NoiseMaker rather than NoiseLocation.  If false, use NoiseLocation.
	 * @param NoiseMaker - Which actor is the source of the noise.  Not to be confused with the Noise Instigator, which is responsible for the noise (and is the pawn on which this function is called).  If not specified, the pawn instigating the noise will be used as the NoiseMaker
	*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=AI)
	void PawnMakeNoise(float Loudness, FVector NoiseLocation, bool bUseNoiseMakerLocation = true, AActor* NoiseMaker = NULL);

	/** default class to use when pawn is controlled by AI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=AI)
	TSubclassOf<class AAIController>  AIControllerClass;

	/** If Pawn is possessed by a player, points to his playerstate.  Needed for network play as controllers are not replicated to clients. */
	UPROPERTY(editinline, replicatedUsing=OnRep_PlayerState)
	class APlayerState* PlayerState;

	/** Replicated so we can see where remote clients are looking. */
	UPROPERTY(replicated)
	uint8 RemoteViewPitch;

	/** Controller of the last Actor that caused us damage. */
	UPROPERTY(transient)
	class AController* LastHitBy;

	/** Controller currently possessing this Actor */
	UPROPERTY(editinline, replicatedUsing=OnRep_Controller)
	class AController* Controller;

	/** Max difference between pawn's Rotation.Yaw and GetDesiredRotation().Yaw for pawn to be considered as having reached its desired rotation */
	float AllowedYawError;

	/** Freeze pawn - stop sounds, animations, physics, weapon firing */
	virtual void TurnOff();

	/** Called when the Pawn is being restarted (usually by being possessed by a Controller). */
	virtual void Restart();

	/** Handle StartFire() passed from PlayerController */
	virtual void PawnStartFire(uint8 FireModeNum = 0);

	/**
	 * Set Pawn ViewPitch, so we can see where remote clients are looking.
	 * Maps 360.0 degrees into a byte
	 * @param	NewRemoteViewPitch	Pitch component to replicate to remote (non owned) clients.
	 */
	void SetRemoteViewPitch(float NewRemoteViewPitch);

	/** Called when our Controller no longer possesses us.	 */
	virtual void UnPossessed();

	/** Return Physics Volume for this Pawn **/
	virtual class APhysicsVolume* GetPawnPhysicsVolume() const;

private:
	/** (DEPRECATED) @RETURN true if Pawn is currently walking (moving along the ground) */
	UFUNCTION(BlueprintCallable, Category="Character|Movement", meta=(DeprecatedFunction, DeprecationMessage="Query the movement component instead using IsMovingOnGround()."))
	virtual bool IsWalking() const;

	/** (DEPRECATED) @RETURN true if Pawn is currently falling */
	UFUNCTION(BlueprintCallable, Category="Character|Movement", meta=(DeprecatedFunction, DeprecationMessage="Query the movement component instead."))
	virtual bool IsFalling() const;

	/** (DEPRECATED) @RETURN true is Pawn is currently crouched */
	UFUNCTION(BlueprintCallable, Category="Character|Movement", meta=(DeprecatedFunction, DeprecationMessage="Query the movement component instead."))
	virtual bool IsCrouched() const;

public:
	UFUNCTION(BlueprintPure, Category="Pawn", meta=(ToolTip="Gets the owning actor of the Movement Base Component on which the pawn is standing."))
	static AActor* GetMovementBaseActor(const APawn* Pawn);

	virtual bool ReachedDesiredRotation();

	/** @return The half-height of the default Pawn, scaled by the component scale. By default returns the half-height of the RootComponent, regardless of whether it is registered or collidable. */
	virtual float GetDefaultHalfHeight() const;

	/** See if this actor is currently being controlled */
	UFUNCTION(BlueprintCallable, Category="Pawn")
	bool IsControlled() const;

	/** Returns controller for this actor. */
	UFUNCTION(BlueprintCallable, Category="Pawn")
	AController* GetController() const;

	/** Get the rotation of the Controller, often the 'view' rotation of this Pawn. */
	UFUNCTION(BlueprintCallable, Category="Pawn")
	FRotator GetControlRotation() const;

	/** Called when Controller is replicated */
	UFUNCTION()
	virtual void OnRep_Controller();

	/** PlayerState Replication Notification Callback */
	UFUNCTION()
	virtual void OnRep_PlayerState();

	/** used to prevent re-entry of OutsideWorldBounds event. */
	uint32 bProcessingOutsideWorldBounds:1;

	// Begin AActor Interface.
	virtual FVector GetVelocity() const OVERRIDE;
	virtual void Tick(float DeltaSeconds) OVERRIDE;
	virtual void Reset() OVERRIDE;
	virtual FString GetHumanReadableName() const OVERRIDE;
	virtual float GetNetPriority(const FVector& ViewPos, const FVector& ViewDir, APlayerController* Viewer, UActorChannel* InChannel, float Time, bool bLowBandwidth) OVERRIDE;
	virtual bool ShouldTickIfViewportsOnly() const OVERRIDE { return IsLocallyControlled() && Cast<APlayerController>(GetController()); }
	virtual bool IsNetRelevantFor(APlayerController* RealViewer, AActor* Viewer, const FVector& SrcLocation) OVERRIDE;
	virtual void PostNetReceiveLocation() OVERRIDE;
	virtual void PostNetReceiveVelocity(const FVector& NewVelocity) OVERRIDE;
	virtual void DisplayDebug(class UCanvas* Canvas, const TArray<FName>& DebugDisplay, float& YL, float& YPos) OVERRIDE;
	virtual void GetActorEyesViewPoint( FVector& Location, FRotator& Rotation ) const OVERRIDE;
	virtual void OutsideWorldBounds() OVERRIDE;
	virtual void Destroyed() OVERRIDE;
	virtual void PreInitializeComponents() OVERRIDE;
	virtual void PostInitializeComponents() OVERRIDE;
	virtual class UPlayer* GetNetOwningPlayer() OVERRIDE;
	virtual class UNetConnection* GetNetConnection() OVERRIDE;
	virtual void PostInitProperties() OVERRIDE;
	virtual void PostLoad() OVERRIDE;
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser) OVERRIDE;
	virtual void BecomeViewTarget(class APlayerController* PC) OVERRIDE;
	virtual bool UpdateNavigationRelevancy() OVERRIDE { SetNavigationRelevancy(false); return false; }
	virtual void EnableInput(class APlayerController* PlayerController) OVERRIDE;
	virtual void DisableInput(class APlayerController* PlayerController) OVERRIDE;

	/** Overridden to defer to the RootComponent's CanBeCharacterBase setting if it is explicitly Yes or No. If set to Owner, will return Super::CanBeBaseForCharacter(). */
	virtual bool CanBeBaseForCharacter(class APawn* APawn) const OVERRIDE;
	// End AActor Interface

	// Begin INavAgentInterface Interface
	virtual const struct FNavAgentProperties* GetNavAgentProperties() const OVERRIDE { return GetMovementComponent() ? GetMovementComponent()->GetNavAgentProperties() : NULL;}
	virtual FVector GetNavAgentLocation() const OVERRIDE { return GetActorLocation(); }
	virtual void GetMoveGoalReachTest(class AActor* MovingActor, const FVector& MoveOffset, FVector& GoalOffset, float& GoalRadius, float& GoalHalfHeight) const OVERRIDE;
	// End INavAgentInterface Interface

	/** @return true if we are in a state to take damage (checked at the start of TakeDamage.
	*   Subclasses may check this as well if they override TakeDamage and don't want to potentially trigger TakeDamage actions by checking if it returns zero in the super class. */
	virtual bool ShouldTakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) const;

#if WITH_EDITOR
	virtual void EditorApplyRotation(const FRotator& DeltaRotation, bool bAltDown, bool bShiftDown, bool bCtrlDown);
#endif

	/** @return vector direction of gravity */
	FVector GetGravityDirection();

	/** Make sure pawn properties are back to default. */
	virtual void SetPlayerDefaults();

	/** Set BaseEyeHeight based on current state. */
	virtual void RecalculateBaseEyeHeight();

	bool InputEnabled() const { return bInputEnabled; }

	/** 
	 * Called when this Pawn is possessed. Only called on the server (or in standalone).
	 *	@param C is the controller possessing this pawn
	 */
	virtual void PossessedBy(class AController* NewController);

	UFUNCTION(BlueprintImplementableEvent, meta=(FriendlyName = "Possessed"))
	void ReceivePossessed(AController* NewController);

	UFUNCTION(BlueprintImplementableEvent, meta=(FriendlyName = "Unpossessed"))
	void ReceiveUnpossessed(AController* OldController);

	/** @return true if controlled by a local (not network) Controller.	 */
	UFUNCTION(BlueprintCallable, Category="Pawn")
	virtual bool IsLocallyControlled() const;

	/** @return the rotation the Pawn is looking */
	virtual FRotator GetViewRotation() const;

	/** @return	Pawn's eye location */
	virtual FVector GetPawnViewLocation() const;

	/** @return base Aim Rotation without any adjustment (no aim error, no autolock, no adhesion.. just clean initial aim rotation!) */
	virtual FRotator GetBaseAimRotation() const;

	/** return true if player is viewing this Pawn in FreeCam */
	virtual bool InFreeCam() const;

	/** Tell client that the Pawn is begin restarted. Calls Restart(). */
	virtual void PawnClientRestart();

	/** Replicated function to set the pawn rotation, allowing the server to force. */
	virtual void ClientSetRotation( FRotator NewRotation );

	/** Updates Pawn's rotation to the given rotation, assumed to be the Controller's ControlRotation. Respects the bUseControllerRotation* settings. */
	virtual void FaceRotation(FRotator NewControlRotation, float DeltaTime = 0.f);

	/** Call this function to detach safely pawn from its controller, knowing that we will be destroyed soon.	 */
	UFUNCTION(BlueprintCallable, Category="Pawn", meta=(Keywords = "Delete"))
	virtual void DetachFromControllerPendingDestroy();

	/** Spawn default controller for this Pawn, and get possessed by it. */
	UFUNCTION(BlueprintCallable, Category="Pawn")
	virtual void SpawnDefaultController();

protected:
	/** Get the controller instigating the damage. If the damage is caused by the world and the supplied controller is NULL or is this pawn's controller, uses LastHitBy as the instigator. */
	virtual class AController* GetDamageInstigator(class AController* InstigatedBy, const class UDamageType& DamageType) const;

	/** Creates an InputComponent that can be used for custom input bindings. Called upon possession by a PlayerController. Return null if you don't want one. */
	virtual class UInputComponent* CreatePlayerInputComponent();

	/** Destroys the player input component and removes any references to it. */
	virtual void DestroyPlayerInputComponent();

	/** Allows a Pawn to set up custom input bindings. Called upon possession by a PlayerController, using the InputComponent created by CreatePlayerInputComponent(). */
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) { /* No bindings by default.*/ }

public:
	/**
	 * Add movement input to our PawnMovementComponent, along the given world direction vector scaled by 'ScaleValue'. If ScaleValue < 0, movement will be in the opposite direction.
	 *
	 * Note that input is accumulated on the movement component (accessed via GetInputVector()) during input processing,
	 * and actual movement of the Pawn won't occur until the acceleration is applied.
	 */
	UFUNCTION(BlueprintCallable, Category="Pawn")
	virtual void AddMovementInput(FVector WorldDirection, float ScaleValue = 1.0f);

	/** Add input (affecting Pitch) to the Controller's ControlRotation, if it is a local PlayerController. */
	UFUNCTION(BlueprintCallable, Category="Pawn", meta=(Keywords="up down"))
	virtual void AddControllerPitchInput(float Val);

	/** Add input (affecting Yaw) to the Controller's ControlRotation, if it is a local PlayerController. */
	UFUNCTION(BlueprintCallable, Category="Pawn", meta=(Keywords="left right turn"))
	virtual void AddControllerYawInput(float Val);

	/** Add input (affecting Roll) to the Controller's ControlRotation, if it is a local PlayerController. */
	UFUNCTION(BlueprintCallable, Category="Pawn")
	virtual void AddControllerRollInput(float Val);

public:
	/** (DEPRECATED) Launch Character with LaunchVelocity  */
	UFUNCTION(BlueprintCallable, Category="Pawn", meta=(DeprecatedFunction, DeprecationMessage="Use Character.LaunchCharacter instead"))
	void LaunchPawn(FVector LaunchVelocity, bool bXYOverride, bool bZOverride);

	/** Add an Actor to ignore by Pawn's movement collision */
	void MoveIgnoreActorAdd(AActor* ActorToIgnore);

	/** Remove an Actor to ignore by Pawn's movement collision */
	void MoveIgnoreActorRemove(AActor* ActorToIgnore);
};


//////////////////////////////////////////////////////////////////////////
// Inlines

inline class UPawnMovementComponent* APawn::GetMovementComponent() const
{
	return FindComponentByClass<UPawnMovementComponent>();
}
