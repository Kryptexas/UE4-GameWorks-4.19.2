// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Movement component updates position of associated PrimitiveComponent (owned by a Character) during its tick.
 * Assumes/requires the character's root component to be a CapsuleComponent.
 */

#pragma once
#include "../AI/AITypes.h"
#include "CharacterMovementComponent.generated.h"

/** Movement modes for Characters.  */
UENUM(BlueprintType)
enum EMovementMode
{
	MOVE_None		UMETA(DisplayName="None"),
	MOVE_Walking	UMETA(DisplayName="Walking"),
	MOVE_Falling	UMETA(DisplayName="Falling"),
	MOVE_Swimming	UMETA(DisplayName="Swimming"),
	MOVE_Flying		UMETA(DisplayName="Flying"),
	MOVE_Custom		UMETA(DisplayName="Custom"),
	MOVE_Unused		UMETA(Hidden),
	MOVE_MAX		UMETA(Hidden),
};


// Data about the floor for walking movement.
USTRUCT(BlueprintType)
struct FFindFloorResult
{
	GENERATED_USTRUCT_BODY()

	/** True if there was a blocking hit in the floor test. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category=CharacterFloor)
	uint32 bBlockingHit:1;

	/** True if the hit found a valid walkable floor. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category=CharacterFloor)
	uint32 bWalkableFloor:1;

	/** True if the hit found a valid walkable floor using a line trace (rather than a sweep test, which happens when the sweep test fails to yield a walkable surface). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category=CharacterFloor)
	uint32 bLineTrace:1;

	/** The distance to the floor, computed from the swept capsule trace. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category=CharacterFloor)
	float FloorDist;
	
	/** The distance to the floor, computed from the trace. Only valid if bLineTrace is true. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category=CharacterFloor)
	float LineDist;

	/** Hit result of the test that found a floor. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category=CharacterFloor)
	FHitResult HitResult;

public:

	FFindFloorResult()
		: bBlockingHit(false)
		, bWalkableFloor(false)
		, bLineTrace(false)
		, FloorDist(0.f)
		, LineDist(0.f)
		, HitResult(1.f)
	{
	}

	/** Returns true if the floor result hit a walkable surface. */
	bool IsWalkableFloor() const
	{
		return bBlockingHit && bWalkableFloor;
	}

	void Clear()
	{
		bBlockingHit = false;
		bWalkableFloor = false;
		bLineTrace = false;
		FloorDist = 0.f;
		LineDist = 0.f;
		HitResult.Reset(1.f, false);
	}

	void SetFromSweep(const FHitResult& InHit, const float InSweepFloorDist, const bool bIsWalkableFloor);
	void SetFromLineTrace(const FHitResult& InHit, const float InSweepFloorDist, const float InLineDist, const bool bIsWalkableFloor);
};


/** Utility struct to accumulate root motion. */
USTRUCT()
struct FRootMotionMovementParams
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	bool bHasRootMotion;

	UPROPERTY()
	FTransform RootMotionTransform;

	FRootMotionMovementParams()
		: bHasRootMotion(false)
		, RootMotionTransform(FTransform::Identity)
	{
	}

	void Set(const FTransform & InTransform)
	{
		bHasRootMotion = true;
		RootMotionTransform = InTransform;
	}

	void Accumulate(const FTransform & InTransform)
	{
		if( !bHasRootMotion )
		{
			Set(InTransform);
		}
		else
		{
			RootMotionTransform = InTransform * RootMotionTransform;
		}
	}

	void Accumulate(const FRootMotionMovementParams & MovementParams)
	{
		if( MovementParams.bHasRootMotion )
		{
			Accumulate(MovementParams.RootMotionTransform);
		}
	}

	void Clear()
	{
		bHasRootMotion = false;
	}
};


UCLASS(HeaderGroup=Component, dependson=(UNetworkPredictionInterface))
class ENGINE_API UCharacterMovementComponent : public UPawnMovementComponent, public INetworkPredictionInterface
{
	GENERATED_UCLASS_BODY()

protected:

	/** Character movement component belongs to */
	UPROPERTY()
	class ACharacter* CharacterOwner;

public:

	/** Maximum height character can step up */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	float MaxStepHeight;

	/** Initial velocity (instantaneous vertical acceleration) when jumping. */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite, meta=(DisplayName="Jump Z Velocity"))
	float JumpZVelocity;

	/** Fraction of JumpZVelocity to use when automatically "jumping off" of a base actor that's not allowed to be a base for a character. (For example, if you're not allowed to stand on other players.) */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite, AdvancedDisplay)
	float JumpOffJumpZFactor;

private:

	/**
	 * Max angle in degrees of a walkable surface. Any greater than this and it is too steep to be walkable.
	 */
	UPROPERTY(Category="Character Movement", EditAnywhere, meta=(ClampMin="0.0", ClampMax="90.0", UIMin = "0.0", UIMax = "90.0"))
	float WalkableFloorAngle;

	/**
	 * Minimum Z value for floor normal. If less, not a walkable surface. Computed from WalkableFloorAngle.
	 */
	UPROPERTY(Category="Character Movement", VisibleAnywhere)
	float WalkableFloorZ;

public:
	
	/**
	 * Don't allow the character to perch on the edge of a surface if the contact is this close to the edge of the capsule.
	 * Note that characters will not fall off if they are within MaxStepHeight of a walkable surface below.
	 */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite, AdvancedDisplay)
	float PerchRadiusThreshold;

	/**
	 * When perching on a ledge, add this additional distance to MaxStepHeight when determining how high above a walkable floor we can perch.
	 * Note that we still enforce MaxStepHeight to start the step up; this just allows the character to hang off the edge or step slightly higher off the floor.
	 * (@see PerchRadiusThreshold)
	 */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite, AdvancedDisplay)
	float PerchAdditionalHeight;

	/** Actor's current physics mode. */
	UPROPERTY(Category=MovementMode, BlueprintReadOnly)
	TEnumAsByte<enum EMovementMode> MovementMode;

	/** Save Base location to check in UpdateBasedMovement() whether base moved in the last frame, and therefore pawn needs an update. */
	UPROPERTY()
	FVector OldBaseLocation;

	/** Save Base rotation to check in UpdateBasedMovement() whether base moved in the last frame, and therefore pawn needs an update. */
	UPROPERTY()
	FRotator OldBaseRotation;

	/** Custom gravity scale. Gravity is multiplied by this amount for the character. */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	float GravityScale;

	/** Coefficient of friction. This property allows you to control how much friction pawns using CharacterMovement move across the ground. This can be used to simulate slippery surfaces such as ice or oil by changing the value (possibly based on the material pawn is standing on). */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	float GroundFriction;

	/** The maximum ground speed. Also determines maximum lateral speed when falling. */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	float MaxWalkSpeed;

	/** Multiplier to max ground speed to use when crouched */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	float CrouchedSpeedMultiplier;

	/** Collision half-height when crouching (component scale is applied separately) */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadOnly)
	float CrouchedHalfHeight;

	/** The maximum swimming speed. */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	float MaxSwimSpeed;

	/** The maximum flying speed. */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	float MaxFlySpeed;

	/** Water buoyancy. A ratio (1.0 = neutral buoyancy, 0.0 = no buoyancy) */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	float Buoyancy;

	/** Change in rotation per second, used when UseControllerDesiredRotation or OrientRotationToMovement are true. */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	FRotator RotationRate;

	/** If true, rotate the Character toward the Controller's desired rotation, using RotationRate as the rate of rotation change. */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite, AdvancedDisplay)
	uint32 bUseControllerDesiredRotation:1;

	/**
	 * If true, rotate the Character toward the direction of acceleration, using RotationRate as the rate of rotation change. Overrides UseControllerDesiredRotation.
	 * Normally you will want to make sure that other settings are cleared, such as bUseControllerRotationYaw on the Character.
	 */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	uint32 bOrientRotationToMovement:1;

	/** true during movement update. Attempts to change CharacterOwner and UpdatedComponent are deferred until after an update. */
	UPROPERTY()
	uint32 bMovementInProgress:1;

	/** If true, high-level movement updates will be wrapped in a movement scope that accumulates updates and defers a bulk of the work until the end. */
	UPROPERTY(Category="Character Movement", EditAnywhere, AdvancedDisplay)
	uint32 bEnableScopedMovementUpdates:1;

	/** Ignores Acceleration component, and forces max acceleration to drive APawn at full velocity. */
	UPROPERTY()
	uint32 bForceMaxAccel:1;    

	/** When there is no Controller, Walking Physics abort and force a velocity and acceleration of 0. Set this to true to override. */
	UPROPERTY()
	uint32 bRunPhysicsWithNoController:1;

	/**
	 * Force the Character in MOVE_Walking to do a check for a valid floor even if he hasn't moved. Cleared after next floor check.
	 * Normally if bAlwaysCheckFloor is false we try to avoid the floor check unless some conditions are met, but this can be used to force the next check to always run.
	 */
	UPROPERTY(Category="Character Movement", VisibleInstanceOnly, BlueprintReadWrite, AdvancedDisplay)
	uint32 bForceNextFloorCheck:1;

	/** If true, the capsule needs to be shrunk on this simulated proxy, to avoid replication rounding putting us in geometry.
	  * Whenever this is set to true, this will cause the capsule to be shrunk again on the next update, and then set to false. */
	UPROPERTY()
	uint32 bShrinkProxyCapsule:1;

	/** if true, Character can walk off a ledge. */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	uint32 bCanWalkOffLedges:1;

	/** if true, Character can walk off a ledge when crouching. */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	uint32 bCanWalkOffLedgesWhenCrouching:1;

public:

	/** true to update CharacterOwner and UpdatedComponent after movement ends */
	UPROPERTY()
	uint32 bDeferUpdateMoveComponent:1;

	/** What to update CharacterOwner and UpdatedComponent after movement ends */
	UPROPERTY()
	class UPrimitiveComponent* DeferredUpdatedMoveComponent;

	/** Maximum step height for getting out of water */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite, AdvancedDisplay)
	float MaxOutOfWaterStepHeight;

	/** Z velocity applied when pawn tries to get out of water */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite, AdvancedDisplay)
	float OutofWaterZ;

	/** Amount of lateral movement control available to the pawn when falling. 0 = no control, 1 = full control at max speed of MaxWalkSpeed. */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	float AirControl;

	/** Friction to apply to lateral air movement when falling. */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	float FallingLateralFriction;

	/** Mass of pawn (for when momentum is imparted to it). */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	float Mass;
	
	/** If enabled, the player will interact with physics objects when walking into them. */
	UPROPERTY(Category="Agent Physics", EditAnywhere, BlueprintReadWrite)
	bool bEnablePhysicsInteraction;

	/** If enabled, the TouchForceFactor is applied per kg mass of the affected object. */
	UPROPERTY(Category="Agent Physics", EditAnywhere, BlueprintReadWrite, meta=(editcondition = "bEnablePhysicsInteraction"))
	bool bTouchForceScaledToMass;

	/** If enabled, the PushForceFactor is applied per kg mass of the affected object. */
	UPROPERTY(Category="Agent Physics", EditAnywhere, BlueprintReadWrite, meta=(editcondition = "bEnablePhysicsInteraction"))
	bool bPushForceScaledToMass;

	/** If enabled, the applied push force will try to get the physics object to the same velocity than the player, not faster. This will only
		scale the force down, it will never apply more force than defined by PushForceFactor. */
	UPROPERTY(Category="Agent Physics", EditAnywhere, BlueprintReadWrite, meta=(editcondition = "bEnablePhysicsInteraction"))
	bool bScalePushForceToVelocity;

	/** Initial impulse force to apply when the player bounces into a blocking physics object. */
	UPROPERTY(Category="Agent Physics", EditAnywhere, BlueprintReadWrite, meta=(editcondition = "bEnablePhysicsInteraction"))
	float InitialPushForceFactor;

	/** Force to apply when the player collides with a blocking physics object. */
	UPROPERTY(Category="Agent Physics", EditAnywhere, BlueprintReadWrite, meta=(editcondition = "bEnablePhysicsInteraction"))
	float PushForceFactor;

	/** Z-Offset for the position the force is applied to. 0.0f is the center of the physics object, 1.0f is the top and -1.0f is the bottom of the object. */
	UPROPERTY(Category="Agent Physics", EditAnywhere, BlueprintReadWrite, meta=(UIMin = "-1.0", UIMax = "1.0"), meta=(editcondition = "bEnablePhysicsInteraction"))
	float PushForcePointZOffsetFactor;

	/** Force to apply to physics objects that are touched by the player. */
	UPROPERTY(Category="Agent Physics", EditAnywhere, BlueprintReadWrite, meta=(editcondition = "bEnablePhysicsInteraction"))
	float TouchForceFactor;

	/** Minimum Force applied to touched physics objects. If < 0.0f, there is no minimum. */
	UPROPERTY(Category="Agent Physics", EditAnywhere, BlueprintReadWrite, meta=(editcondition = "bEnablePhysicsInteraction"))
	float MinTouchForce;

	/** Maximum force applied to touched physics objects. If < 0.0f, there is no maximum. */
	UPROPERTY(Category="Agent Physics", EditAnywhere, BlueprintReadWrite, meta=(editcondition = "bEnablePhysicsInteraction"))
	float MaxTouchForce;

	/** Force per kg applied constanly to all overlapping components. */
	UPROPERTY(Category="Agent Physics", EditAnywhere, BlueprintReadWrite, meta=(editcondition = "bEnablePhysicsInteraction"))
	float RepulsionForce;


public:
	// Deprecated properties

	UPROPERTY()
	uint32 bForceBraking_DEPRECATED:1;

protected:

	/**
	 * Current acceleration vector (with magnitude).
	 * This is calculated each update based on the input vector and the constraints of MaxAcceleration and the current movement mode.
	 */
	UPROPERTY()
	FVector Acceleration;

public:

	/** Max Acceleration (rate of change of velocity) */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	float MaxAcceleration;

	/** Deceleration when walking and not applying acceleration. */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	float BrakingDecelerationWalking;

	/** Lateral deceleration when falling and not applying acceleration. */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	float BrakingDecelerationFalling;

	/** Deceleration when swimming and not applying acceleration. */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	float BrakingDecelerationSwimming;

	/** Deceleration when flying and not applying acceleration. */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	float BrakingDecelerationFlying;

	/** Used in determining if pawn is going off ledge.  If the ledge is "shorter" than this value then the pawn will be able to walk off it. **/
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite, AdvancedDisplay)
	float LedgeCheckThreshold;

	/** When exiting water, jump if control pitch angle is this high or above. */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite, AdvancedDisplay)
	float JumpOutOfWaterPitch;

	/**
	 * Impacts on the upper hemisphere will have the vertical velocity scaled by this factor of the Normal's Z component (in AdjustUpperHemisphereImpact).
	 * A value of 0 indicates that this behavior is disabled.
	 */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite, AdvancedDisplay)
	float UpperImpactNormalScale;

	/** Information about the floor the Character is standing on (only used by MOVE_Walking) */
	UPROPERTY(Category="Character Movement", VisibleInstanceOnly, BlueprintReadOnly)
	FFindFloorResult CurrentFloor;

	/** Default movement mode when not in water */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<enum EMovementMode> DefaultLandMovementMode;

	/** Default movement mode when in water */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<enum EMovementMode> DefaultWaterMovementMode;

	/**
	 * If true, walking movement always maintains horizontal velocity when moving up ramps, which causes movement up ramps to be faster parallel to the ramp surface.
	 * If false, then walking movement maintains velocity magnitude parallel to the ramp surface.
	 */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	uint32 bMaintainHorizontalGroundVelocity:1;

	/** If true, impart the base actor's X velocity when falling off it (which includes jumping) */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	uint32 bImpartBaseVelocityX:1;

	/** If true, impart the base actor's Y velocity when falling off it (which includes jumping) */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	uint32 bImpartBaseVelocityY:1;

	/** If true, impart the base actor's Z velocity when falling off it (which includes jumping) */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	uint32 bImpartBaseVelocityZ:1;

	/** If true, impart the base actor's angular velocity when falling off it (which includes jumping).
	  * Only those components of the velocity allowed by the separate component settings (bImpartBaseVelocity[XYZ]) will be applied. */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite)
	uint32 bImpartBaseAngularVelocity:1;

	/** Used by movement code to determine if a change in position is based on normal movement or a teleport. If not a teleport, velocity can be recomputed based on the change in position. */
	UPROPERTY(Transient)
	uint32 bJustTeleported:1;

	/** if true, event NotifyJumpApex() to CharacterOwner's controller when at apex of jump.  Is cleared when event is triggered. */
	UPROPERTY()
	uint32 bNotifyApex:1;

	/** instantly stop in flying mode */
	UPROPERTY()
	uint32 bCheatFlying:1;

	/** If true, try to crouch (or keep crouching). If false, stop crouching if crouched. */
	UPROPERTY()
	uint32 bWantsToCrouch:1;

	/**
	 * If true, crouching should lower the center of the player and keep the base of the capsule in place. If false, the base of the capsule moves up and the center stays in place.
	 * By default, this is set to true when walking and false otherwise when the movement mode changes. Feel free to override the behavior when the movement mode changes.
	 */
	UPROPERTY(Category="Character Movement", VisibleAnywhere, BlueprintReadWrite, AdvancedDisplay)
	uint32 bCrouchMovesCharacterDown:1;

	/** If true, the pawn ignores the effects of changes in its base's rotation on its rotation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Attachment)
	uint32 bIgnoreBaseRotation:1;

	/** 
	 *  Set this to true if riding on a moving base that you know is clear from non-moving world obstructions.
	 *  This can solve move-order dependencies when riding an interp actor, and it's faster.
	 */
	UPROPERTY()
	uint32 bFastAttachedMove:1;

	/**
	 * True to always force floor checks for non-moving Characters.
	 * Normally floor checks are avoided if possible, but this can be used to force them if there are use-cases where they are being skipped erroneously.
	 */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite, AdvancedDisplay)
	uint32 bAlwaysCheckFloor:1;

	/** Used to prevent reentry of JumpOff() */
	UPROPERTY()
	uint32 bPerformingJumpOff:1;

	/** If set, component will use RVO avoidance */
	UPROPERTY(Category="Avoidance", EditAnywhere, BlueprintReadOnly)
	uint32 bUseRVOAvoidance:1;

	/** Should use acceleration for path following? */
	UPROPERTY(Category="Character Movement", EditAnywhere, BlueprintReadWrite, AdvancedDisplay)
	uint32 bRequestedMoveUseAcceleration:1;

protected:

	/** Was velocity requested by path following? */
	UPROPERTY(Transient)
	uint32 bHasRequestedVelocity:1;

	/** Was acceleration requested to be always max speed? */
	UPROPERTY(Transient)
	uint32 bRequestedMoveWithMaxSpeed:1;

	/** Was avoidance updated in this frame? */
	UPROPERTY(Transient)
	uint32 bWasAvoidanceUpdated : 1;

	/** if set, PostProcessAvoidanceVelocity will be called */
	uint32 bUseRVOPostProcess : 1;

	/** forced avoidance velocity, used when AvoidanceLockTimer is > 0 */
	FVector AvoidanceLockVelocity;

	/** remaining time of avoidance velocity lock */
	float AvoidanceLockTimer;

public:

	/** Velocity requested by path following */
	UPROPERTY(Transient)
	FVector RequestedVelocity;

	/** No default value, for now it's assumed to be valid if GetAvoidanceManager() returns non-NULL. */
	UPROPERTY(Category="Avoidance", VisibleAnywhere, BlueprintReadOnly, AdvancedDisplay)
	int32 AvoidanceUID;

	/** Moving actor's group mask */
	UPROPERTY(Category="Avoidance", EditAnywhere, BlueprintReadOnly, AdvancedDisplay)
	FNavAvoidanceMask AvoidanceGroup;
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|CharacterMovement")
	void SetAvoidanceGroup(int32 GroupFlags);

	/** Will avoid other agents if they are in one of specified groups */
	UPROPERTY(Category="Avoidance", EditAnywhere, BlueprintReadOnly, AdvancedDisplay)
	FNavAvoidanceMask GroupsToAvoid;
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|CharacterMovement")
	void SetGroupsToAvoid(int32 GroupFlags);

	/** Will NOT avoid other agents if they are in one of specified groups, higher priority than GroupsToAvoid */
	UPROPERTY(Category="Avoidance", EditAnywhere, BlueprintReadOnly, AdvancedDisplay)
	FNavAvoidanceMask GroupsToIgnore;
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|CharacterMovement")
	void SetGroupsToIgnore(int32 GroupFlags);

	/** De facto default value 0.5 (due to that being the default in the avoidance registration function), indicates RVO behavior. */
	UPROPERTY(Category="Avoidance", EditAnywhere, BlueprintReadOnly)
	float AvoidanceWeight;

	/** Temporarily holds launch velocity when pawn is to be launched so it happens at end of movement. */
	UPROPERTY()
	FVector PendingLaunchVelocity;

	/** Get the Character that owns UpdatedComponent. */
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|CharacterMovement")
	class ACharacter* GetCharacterOwner() const;

	/** Change movement mode */
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|CharacterMovement")
	virtual void SetMovementMode(enum EMovementMode NewMovementMode);

	//Begin UActorComponent Interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) OVERRIDE;
	virtual void OnUnregister() OVERRIDE;
	virtual void PostLoad() OVERRIDE;
	//End UActorComponent Interface

	//BEGIN UMovementComponent Interface
	virtual float GetMaxSpeed() const OVERRIDE;
	virtual float GetMaxSpeedModifier() const OVERRIDE;
	virtual void StopActiveMovement() OVERRIDE;
	virtual bool IsCrouching() const OVERRIDE;
	virtual bool IsFalling() const OVERRIDE;
	virtual bool IsMovingOnGround() const OVERRIDE;
	virtual bool IsSwimming() const OVERRIDE;
	virtual bool IsFlying() const OVERRIDE;
	virtual float GetGravityZ() const OVERRIDE;
	//END UMovementComponent Interface

	//BEGIN UNavMovementComponent Interface
	virtual void RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed) OVERRIDE;
	virtual bool CanStopPathFollowing() const OVERRIDE;
	//END UNaVMovementComponent Interface

	//Begin UPawnMovementComponent Interface
	virtual void NotifyBumpedPawn(APawn* BumpedPawn) OVERRIDE;
	//End UPawnMovementComponent Interface

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR

	/** Make movement impossible (sets movement mode to MOVE_None). */
	virtual void DisableMovement();

	/** Update Velocity and Acceleration to air control in the desired Direction for AIControlled Pawn 
		@PARAM Direction is the desired direction of movement
		@Param ZDiff is the height difference between the destination and the Pawn's current position */
	virtual void PerformAirControl(FVector Direction, float ZDiff);

	/** Transition from walking to falling */
	virtual void StartFalling(int32 Iterations, float remainingTime, float timeTick, const FVector& Delta, const FVector& subLoc);

	/** Whether pawn should go into falling mode when starting down a steep decline.
	 *  @returns false to provide traditional default behavior
	  */
	virtual bool ShouldCatchAir(const FVector& OldFloor, const FVector& Floor);

	/** Adjust distance from floor, trying to maintain a slight offset from the floor when walking (based on CurrentFloor). */
	virtual void AdjustFloorHeight();

	/** Update position based on Base movement */
	virtual void UpdateBasedMovement(float DeltaSeconds);

	/** Update controller's view rotation as pawn's base rotates */
	virtual void UpdateBasedRotation(FRotator &FinalRotation, const FRotator& ReducedRotation);

	/** changes physics based on MovementMode */
	virtual void StartNewPhysics(float deltaTime, int32 Iterations);
	
	/** Perform jump. Note that you should usually trigger a jump through Character::Jump() instead. */
	virtual bool DoJump();

	/** Launch player with LaunchVel. */
	virtual void Launch(FVector const& LaunchVel);

	/**
	 * If we have a movement base, get the velocity that should be imparted by that base, usually when jumping off of it.
	 * Only applies the components of the velocity enabled by bImpartBaseVelocityX, bImpartBaseVelocityY, bImpartBaseVelocityZ.
	 */
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|CharacterMovement")
	virtual FVector GetImpartedMovementBaseVelocity() const;

	/** Force this pawn to bounce off its current base, which isn't an acceptable base for it. */
	virtual void JumpOff(AActor* MovementBaseActor);

	/** Can be overridden to choose to jump based on character velocity, base actor dimensions, etc. */
	virtual FVector GetBestDirectionOffActor(AActor* BaseActor) const; // Calculates the best direction to go to "jump off" an actor.

	/** 
	 * Determine whether the Character should jump when exiting water.
	 * @param	JumpDir is the desired direction to jump out of water
	 * @return	true if Pawn should jump out of water
	 */
	virtual bool ShouldJumpOutOfWater(FVector& JumpDir);

	/** Jump onto shore from water */
	virtual void JumpOutOfWater(FVector WallNormal);

	/** @return how far to rotate character during the time interval DeltaTime. */
	virtual FRotator GetDeltaRotation(float DeltaTime);

	/**
	  * Compute a target rotation based on current movement. Used by PhysicsRotation() when bOrientRotationToMovement is true.
	  * Default implementation targets a rotation based on Acceleration.
	  *
	  * @param CurrentRotation	- Current rotation of the Character
	  * @param DeltaTime		- Time slice for this movement
	  * @param DeltaRotation	- Proposed rotation change based simply on DeltaTime * RotationRate
	  *
	  * @return The target rotation given current movement.
	  */
	virtual FRotator ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation);

	/** use velocity requested by path following */
	virtual bool ApplyRequestedMove(FVector& NewVelocity, FVector& NewAcceleration, float DeltaTime, float MaxAccel, float MaxSpeed);

	/** Called if bNotifyApex is true and character has just passed the apex of its jump. */
	virtual void NotifyJumpApex();

	/** @return new falling velocity from old velocity and acceleration. */
	virtual FVector NewFallVelocity(FVector OldVelocity, FVector OldAcceleration, float timeTick);

	/* Determine how deep in water the character is immersed.
	 * @return float in range 0.0 = not in water, 1.0 = fully immersed
	 */
	virtual float ImmersionDepth();

	/** 
	 * Calculates new velocity and acceleration for pawn for this tick, bounds acceleration and velocity, and adding effects of friction.
	 *
	 * @param	DeltaTime						time elapsed since last frame.
	 * @param	Friction						coefficient of friction when not accelerating, or in the direction opposite of acceleration.
	 * @param	bFluid							true if moving through a fluid, causing Friction to always be applied regardless of acceleration.
	 * @param	BrakingDeceleration				deceleration applied when not accelerating, or when exceeding max velocity.
	 */
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration);
	
	/** Compute the max jump height based on the JumpZVelocity velocity and gravity. */
	virtual float GetMaxJumpHeight() const;
	
	/** @return Maximum acceleration for the current state, based on MaxAcceleration and GetMaxSpeedModifier() */
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|CharacterMovement")
	virtual float GetModifiedMaxAcceleration() const;

	/** @return Current acceleration, computed from input vector each update. */
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|CharacterMovement")
	FVector GetCurrentAcceleration() const;

	/** @return true if we can step up on the actor in the given FHitResult. */
	virtual bool CanStepUp(const FHitResult& Hit) const;

	/**  Struct updated by StepUp() method to return result of final step down, if applicable. */
	struct FStepDownResult
	{
		uint32 bComputedFloor:1;		// True if the floor was computed as a result of the step down.
		FFindFloorResult FloorResult;	// The result of the floor test if the floor was updated.

		FStepDownResult()
			: bComputedFloor(false)
		{
		}
	};

	/** 
	 * Move up steps or slope. Does nothing and returns false if CanStepUp(Hit) returns false.
	 *
	 * @param GravDir:			Gravity vector
	 * @param Delta:			Requested move
	 * @param Hit:				[In] The hit before the step up.
	 * @param OutStepDownResult:[Out] If non-null, a floor check will be performed if possible as part of the final step down, and it will be updated to reflect this result.
	 * @return true if the step up was successful.
	 */
	virtual bool StepUp(const FVector& GravDir, const FVector& Delta, const FHitResult &Hit, struct UCharacterMovementComponent::FStepDownResult* OutStepDownResult = NULL);

	/** Update the base of the actor */
	virtual void SetBase(UPrimitiveComponent* NewBase, bool bNotifyActor=true);

	/** Applies repulsion force to all touched components */
	void ApplyRepulsionForce(float DeltaTime);

public:

	/** 
	 * Handle start swimming functionality
	 * @param OldLocation - Location on last tick
	 * @param OldVelocity - velocity at last tick
	 * @param timeTick - time since at OldLocation
	 * @param remainingTime - DeltaTime to complete transition to swimming
	 * @param Iterations - physics iteration count
	 */
	void StartSwimming(FVector OldLocation, FVector OldVelocity, float timeTick, float remainingTime, int32 Iterations);

	/* Swimming uses gravity - but scaled by (1.f - buoyancy) */
	float Swim(FVector Delta, FHitResult &Hit);

	/** Get as close to waterline as possible, staying on same side as currently. */
	FVector FindWaterLine(FVector Start, FVector End);

	/** Handle changing physics to falling over deltatime and iterations */
	virtual void PhysFalling(float deltaTime, int32 Iterations);

	/** Handle landing against Hit surface over remaingTime and iterations */
	virtual void ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations);

	/** Set new physics after landing */
	virtual void SetPostLandedPhysics(const FHitResult& Hit);

	/**
	 * Checks if new capsule size fits (no encroachment), and call CharacterOwner->OnStartCrouch() if successful.
	 * In general you should set bWantsToCrouch instead to have the crouch persist during movement, or just use the crouch functions on the owning Character.
	 * @param	bClientSimulation	true when called when bIsCrouched is replicated to non owned clients, to update collision cylinder and offset.
	 */
	virtual void Crouch(bool bClientSimulation = false);
	
	/**
	 * Checks if default capsule size fits (no encroachment), and trigger OnEndCrouch() on the owner if successful.
	 * @param	bClientSimulation	true when called when bIsCrouched is replicated to non owned clients, to update collision cylinder and offset.
	 */
	virtual void UnCrouch(bool bClientSimulation = false);

	// @return true if the character is allowed to crouch in the current state. By default it is allowed when walking or falling, if CanEverCrouch() is true.
	virtual bool CanCrouchInCurrentState() const;
	
	/** @return true if there is a suitable floor SideStep from current position  */
	bool CheckLedgeDirection(const FVector& OldLocation, const FVector& SideStep, const FVector& GravDir);

	/** 
	 * @param Delta is the current move delta (which ended up going over a ledge.
	 * @return new delta which moves along the ledge
	 */
	FVector GetLedgeMove(const FVector& OldLocation, const FVector& Delta, const FVector& GravDir);

	/** Check if pawn is falling */
	bool CheckFall(FHitResult &Hit, FVector Delta, FVector subLoc, float remainingTime, float timeTick, int32 Iterations, bool bMustJump);
	
	/** 
	 *  Revert to previous position OldLocation, return to being based on OldBase.
	 *  if bFailMove, stop movement and notify controller
	 */	
	void RevertMove(const FVector& OldLocation, UPrimitiveComponent* OldBase, const FVector& OldBaseLocation, const FFindFloorResult& OldFloor, bool bFailMove);

	/** Perform rotation over deltaTime */
	virtual void PhysicsRotation(float DeltaTime);

	/** Delegate when PhysicsVolume of UpdatedComponent has been changed **/
	virtual void PhysicsVolumeChanged(class APhysicsVolume* NewVolume);

	/** Set movement mode to the default based on the current physics volume. */
	virtual void SetDefaultMovementMode();

	/**
	 * Moves along the given movement direction using simple movement rules based on the current movement mode (usually used by simulated proxies).
	 *
	 * @param InVelocity:			Velocity of movement
	 * @param DeltaSeconds:			Time over which movement occurs
	 * @param OutStepDownResult:	[Out] If non-null, and a floor check is performed, this will be updated to reflect that result.
	 */
	virtual void MoveSmooth(const FVector& InVelocity, const float DeltaSeconds, FStepDownResult* OutStepDownResult = NULL );

	
	virtual void SetUpdatedComponent(class UPrimitiveComponent* NewUpdatedComponent) OVERRIDE;
	
	/** @Return MovementMode string */
	virtual FString GetMovementName();

	/** Add velocity based on imparted momentum */
	virtual void AddMomentum( FVector const& Momentum, FVector const& LocationToApplyMomentum, bool bMassIndependent );

	/**
	 * Draw important variables on canvas.  Character will call DisplayDebug() on the current ViewTarget when the ShowDebug exec is used
	 *
	 * @param Canvas - Canvas to draw on
	 * @param DebugDisplay - List of names specifying what debug info to display
	 * @param YL - Height of the current font
	 * @param YPos - Y position on Canvas. YPos += YL, gives position to draw text for next debug line.
	 */
	virtual void DisplayDebug(class UCanvas* Canvas, const TArray<FName>& DebugDisplay, float& YL, float& YPos);

	/** Check if swimming pawn just ran into edge of the pool and should jump out. */
	virtual bool CheckWaterJump(FVector CheckPoint, FVector& WallNormal);

	/** @return whether this pawn is currently allowed to walk off ledges */
	virtual bool CanWalkOffLedges() const;

	/** @return The distance from the edge of the capsule within which we don't allow the character to perch on the edge of a surface. */
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|CharacterMovement")
	float GetPerchRadiusThreshold() const;

	/**
	 * Returns the radius within which we can stand on the edge of a surface without falling (if this is a walkable surface).
	 * Simply computed as the capsule radius minus the result of GetPerchRadiusThreshold().
	 */
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|CharacterMovement")
	float GetValidPerchRadius() const;

	/** Return true if the hit result should be considered a walkable surface for the character. */
	virtual bool IsWalkable(const FHitResult& Hit) const;


	/** Get the max angle in degrees of a walkable surface for the character. */
	FORCEINLINE float GetWalkableFloorAngle() const { return WalkableFloorAngle; }

	/** Get the max angle in degrees of a walkable surface for the character. */
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|CharacterMovement", meta=(FriendlyName = "GetWalkableFloorAngle"))
	float K2_GetWalkableFloorAngle() const;

	/** Set the max angle in degrees of a walkable surface for the character. Also computes WalkableFloorZ. */
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|CharacterMovement")
	void SetWalkableFloorAngle(float InWalkableFloorAngle);


	/** Get the Z component of the normal of the steepest walkable surface for the character. Any lower than this and it is not walkable. */
	FORCEINLINE float GetWalkableFloorZ() const { return WalkableFloorZ; }

	/** Get the Z component of the normal of the steepest walkable surface for the character. Any lower than this and it is not walkable. */
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|CharacterMovement", meta=(FriendlyName = "GetWalkableFloorZ"))
	float K2_GetWalkableFloorZ() const;

	/** Set the Z component of the normal of the steepest walkable surface for the character. Also computes WalkableFloorAngle. */
	UFUNCTION(BlueprintCallable, Category="Pawn|Components|CharacterMovement")
	void SetWalkableFloorZ(float InWalkableFloorZ);


protected:
	/** @note Movement update functions should only be called through StartNewPhysics()*/
	virtual void PhysWalking(float deltaTime, int32 Iterations);

	/** @note Movement update functions should only be called through StartNewPhysics()*/
	virtual void PhysFlying(float deltaTime, int32 Iterations);

	/** @note Movement update functions should only be called through StartNewPhysics()*/
	virtual void PhysSwimming(float deltaTime, int32 Iterations);

	/** @note Movement update functions should only be called through StartNewPhysics()*/
	virtual void PhysCustom(float deltaTime, int32 Iterations);

	/**
	 * Compute a vector of movement, given a delta and a hit result of the surface we are on.
	 *
	 * @param Delta:				Attempted movement direction
	 * @param RampHit:				Hit result of sweep that found the ramp below the capsule
	 * @param bHitFromLineTrace:	Whether the floor trace came from a line trace
	 *
	 * @return If on a walkable surface, this returns a vector that moves parallel to the surface while maintaining horizontal velocity.
	 * If a ramp vector can't be computed, this will just return Delta.
	 */
	virtual FVector ComputeGroundMovementDelta(const FVector& Delta, const FHitResult& RampHit, const bool bHitFromLineTrace) const;

	/**
	 * Move along the floor, using CurrentFloor and ComputeGroundMovementDelta() to get a movement direction.
	 * If a second walkable surface is hit, it will also be moved along using the same approach.
	 *
	 * @param InVelocity:			Velocity of movement
	 * @param DeltaSeconds:			Time over which movement occurs
	 * @param OutStepDownResult:	[Out] If non-null, and a floor check is performed, this will be updated to reflect that result.
	 */
	virtual void MoveAlongFloor(const FVector& InVelocity, const float DeltaSeconds, FStepDownResult* OutStepDownResult = NULL);

	/**
	 * Adjusts velocity when walking so that Z velocity is zero.
	 * When bMaintainHorizontalGroundVelocity is false, also rescales the velocity vector to maintain the original magnitude, but in the horizontal direction.
	 */
	virtual void MaintainHorizontalGroundVelocity();

	/** Overridden to set bJustTeleported to true, so we don't make incorrect velocity calculations based on adjusted movement. */
	virtual bool ResolvePenetration(const FVector& Adjustment, const FHitResult& Hit, const FRotator& NewRotation) OVERRIDE;

	/** Overridden to avoid upward adjustments that can go too high when walking. */
	virtual FVector GetPenetrationAdjustment(const FHitResult& Hit) const OVERRIDE;

	/** Don't call for intermediate parts of move! */
	virtual void HandleImpact(FHitResult const& Hit, float TimeSlice=0.f, const FVector& MoveDelta = FVector::ZeroVector) OVERRIDE;

	/** Custom version of SlideAlongSurface that handles different movement modes separately; namely during walking physics we might not want to slide up slopes. */
	virtual float SlideAlongSurface(const FVector& Delta, float Time, const FVector& Normal, FHitResult &Hit, bool bHandleImpact) OVERRIDE;

	/** Custom version that allows upwards slides when walking if the surface is walkable. */
	virtual void TwoWallAdjust(FVector &Delta, const FHitResult& Hit, const FVector &OldHitNormal) const OVERRIDE;

	/**
	 * Calculate slide on a slope. Has special treatment when falling, to avoid boosting up slopes.
	 * Calls AdjustUpperHemisphereImpact() for upward falling movement that hits the top of the capsule, because commonly we don't want this to behave like a smooth capsule.
	 */
	virtual FVector ComputeSlideVector(const FVector& Delta, const float Time, const FVector& Normal, const FHitResult& Hit) const OVERRIDE;

	/**
	 * Given an upward impact on the top of the capsule, allows calculation of a different movement delta.
	 * Default implementation scales the Delta Z value by: 1 - (Abs(Normal.Z) * UpperImpactNormalScale), clamped to [0..1]
	 */
	virtual FVector AdjustUpperHemisphereImpact(const FVector& Delta, const FHitResult& Hit) const;

	/** Slows towards stop. */
	virtual void ApplyVelocityBraking(float DeltaTime, float Friction, float BrakingDeceleration);

	/**
	 * Return true if the 2D distance to the impact point is inside the edge tolerance (CapsuleRadius minus a small rejection threshold).
	 * Useful for rejecting adjacent hits when finding a floor or landing spot.
	 */
	bool IsWithinEdgeTolerance(const FVector& CapsuleLocation, const FVector& TestImpactPoint, const float CapsuleRadius) const;

	/**
	 * Sweeps a vertical trace to find the floor for the capsule at the given location. Will attempt to perch if ShouldComputePerchResult() returns true for the downward sweep result.
	 *
	 * @param CapsuleLocation:		Location where the capsule sweep should originate
	 * @param OutFloorResult:		[Out] Contains the result of the floor check.
	 * @param bZeroDelta:			If true, the capsule was not actively moving in this update (can be used to avoid unnecessary floor tests).
	 * @param DownwardSweepResult:	If non-null and it contains valid blocking hit info, this will be used as the result of a downward sweep test instead of doing it as part of the update.
	 */
	virtual void FindFloor(const FVector& CapsuleLocation, struct FFindFloorResult& OutFloorResult, bool bZeroDelta, const FHitResult* DownwardSweepResult = NULL) const;

	/**
	 * Compute distance to the floor from bottom sphere of capsule. This is the swept distance of the capsule to the first point impacted by the lower sphere.
	 * SweepDistance MUST be greater than or equal to the line distance.
	 * @see FindFloor
	 *
	 * @param CapsuleLocation:	Location of the capsule used for the query
	 * @param LineDistance:		If non-zero, max distance to test for a simple line check from the capsule base. Used before the sweep test, and only returns a valid result if the impact normal is a walkable normal.
	 * @param SweepDistance:	If non-zero, max distance to use when sweeping a capsule downwards for the test.
	 * @param OutFloorResult:	Result of the floor check.
	 * @param SweepRadius:		The radius to use for sweep tests. Should be <= capsule radius.
	 * @param DownwardSweepResult:	If non-null and it contains valid blocking hit info, this will be used as the result of a downward sweep test instead of doing it as part of the update.
	 *
	 */
	virtual void ComputeFloorDist(const FVector& CapsuleLocation, float LineDistance, float SweepDistance, FFindFloorResult& OutFloorResult, float SweepRadius, const FHitResult* DownwardSweepResult = NULL) const;

	/** Verify that the supplied hit result is a valid landing spot when falling. */
	virtual bool IsValidLandingSpot(const FVector& CapsuleLocation, const FHitResult& Hit) const;

	/**
	 * Check if the result of a sweep test (passed in InHit) might be a valid location to perch, in which case we should use ComputePerchResult to validate the location.
	 * @see ComputePerchResult
	 * @param InHit:			Result of the last sweep test before this query.
	 * @param bCheckRadius:		If true, only allow the perch test if the impact point is outside the radius returned by GetValidPerchRadius().
	 * @return Whether perching may be possible, such that ComputePerchResult can return a valid result.
	 */
	virtual bool ShouldComputePerchResult(const FHitResult& InHit, bool bCheckRadius = true) const;

	/**
	 * Compute the sweep result of the smaller capsule with radius specified by GetValidPerchRadius(),
	 * and return true if the sweep contacts a valid walkable normal within InMaxFloorDist of InHit.ImpactPoint.
	 * This may be used to determine if the capsule can or cannot stay at the current location if perched on the edge of a small ledge or unwalkable surface.
	 * Note: Only returns a valid result if ShouldComputePerchResult returned true for the supplied hit value.
	 *
	 * @param TestRadius:			Radius to use for the sweep, usually GetValidPerchRadius().
	 * @param InHit:				Result of the last sweep test before the query.
	 * @param InMaxFloorDist:		Max distance to floor allowed by perching, from the supplied contact point (InHit.ImpactPoint).
	 * @param OutPerchFloorResult:	Contains the result of the perch floor test.
	 * @return True if the current location is a valid spot at which to perch.
	 */
	virtual bool ComputePerchResult(const float TestRadius, const FHitResult& InHit, const float InMaxFloorDist, FFindFloorResult& OutPerchFloorResult) const;

	/** Called when the collision capsule touches another primitive component */
	UFUNCTION()
	void CapsuleTouched(AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Enum used to control GetPawnCapsuleExtent behavior
	enum EShrinkCapsuleExtent
	{
		SHRINK_None,			// Don't change the size of the capsule
		SHRINK_RadiusCustom,	// Change only the radius, based on a supplied param
		SHRINK_HeightCustom,	// Change only the height, based on a supplied param
		SHRINK_AllCustom,		// Change both radius and height, based on a supplied param
	};

	/** Get the capsule extent for the Pawn owner, possibly reduced in size depending on ShrinkMode.
	 * @param ShrinkMode			Controls the way the capsule is resized.
	 * @param CustomShrinkAmount	The amount to shrink the capsule, used only for ShrinkModes that specify custom.
	 * @return The capsule extent of the Pawn owner, possibly reduced in size depending on ShrinkMode.
	 */
	FVector GetPawnCapsuleExtent(const EShrinkCapsuleExtent ShrinkMode, const float CustomShrinkAmount = 0.f) const;
	
	/** Get the collision shape for the Pawn owner, possibly reduced in size depending on ShrinkMode.
	 * @param ShrinkMode			Controls the way the capsule is resized.
	 * @param CustomShrinkAmount	The amount to shrink the capsule, used only for ShrinkModes that specify custom.
	 * @return The capsule extent of the Pawn owner, possibly reduced in size depending on ShrinkMode.
	 */
	FCollisionShape GetPawnCapsuleCollisionShape(const EShrinkCapsuleExtent ShrinkMode, const float CustomShrinkAmount = 0.f) const;

	/** Adjust the size of the capsule on simulated proxies, to avoid overlaps due to replication rounding.
	  * Changes to the capsule size on the proxy should set bShrinkProxyCapsule=true and possibly call AdjustProxyCapsuleSize() immediately if applicable.
	  */
	virtual void AdjustProxyCapsuleSize();

	/** Enforce constraints on input given current state. For instance, don't move upwards if walking and looking up. */
	virtual FVector ConstrainInputAcceleration(const FVector& InputAcceleration) const;

	/** Scale input acceleration, based on movement acceleration rate. */
	virtual FVector ScaleInputAcceleration(const FVector& InputAcceleration) const;

public:
	// Movement functions broken out based on owner's network Role.
	// TickComponent calls the correct version based on the Role.
	// These may be called during move playback and correction during network updates.
	//

	/** Perform movement on an autonomous client */
	virtual void PerformMovement(float DeltaTime);

	/** Special Tick for Simulated Proxies */
	void SimulatedTick(float DeltaSeconds);

	/** Simulate movement on a non-owning client. */
	virtual void SimulateMovement(float DeltaTime);

	/** Given current simulated conditions (on simulated proxies or autonomous), determine current movement mode. */
	virtual EMovementMode DetermineSimulatedMovementMode() const;

	/** Force a client update by making it appear on the server that the client hasn't updated in a long time. */
	virtual void ForceReplicationUpdate();
	
	/**
	 * Generate a random angle in degrees that is approximately equal between client and server.
	 * Note that in networked games this result changes with low frequency and has a low period,
	 * so should not be used for frequent randomization.
	 */
	virtual float GetNetworkSafeRandomAngleDegrees() const;

	//--------------------------------
	// INetworkPredictable implementation

	//--------------------------------
	// Server hook
	//--------------------------------
	virtual void SendClientAdjustment() OVERRIDE;
	virtual void ForcePositionUpdate(float DeltaTime) OVERRIDE;

	//--------------------------------
	// Client hook
	//--------------------------------
	virtual void SmoothCorrection(const FVector& OldLocation) OVERRIDE;

	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const OVERRIDE;
	virtual class FNetworkPredictionData_Server* GetPredictionData_Server() const OVERRIDE;

	virtual bool HasPredictionData_Client() const OVERRIDE { return ClientPredictionData != NULL; }
	virtual bool HasPredictionData_Server() const OVERRIDE { return ServerPredictionData != NULL; }

	virtual void ResetPredictionData_Client() OVERRIDE;
	virtual void ResetPredictionData_Server() OVERRIDE;

protected:
	class FNetworkPredictionData_Client_Character* ClientPredictionData;
	class FNetworkPredictionData_Server_Character* ServerPredictionData;

	virtual class FNetworkPredictionData_Client_Character* GetPredictionData_Client_Character() const;
	virtual class FNetworkPredictionData_Server_Character* GetPredictionData_Server_Character() const;

	virtual void SmoothClientPosition(float DeltaTime);

	/*
	========================================================================
	Here's how player movement prediction, replication and correction works in network games:
	
	Every tick, the TickComponent() function is called.  It figures out the acceleration and rotation change for the frame,
	and then calls PerformMovement() (for locally controlled Characters), or ReplicateMoveToServer() (if it's a network client).
	
	ReplicateMoveToServer() saves the move (in the PendingMove list), calls PerformMovement(), and then replicates the move
	to the server by calling the replicated function ServerMove() - passing the movement parameters, the client's
	resultant position, and a timestamp.
	
	ServerMove() is executed on the server.  It decodes the movement parameters and causes the appropriate movement
	to occur.  It then looks at the resulting position and if enough time has passed since the last response, or the
	position error is significant enough, the server calls ClientAdjustPosition(), a replicated function.
	
	ClientAdjustPosition() is executed on the client.  The client sets its position to the servers version of position,
	and sets the bUpdatePosition flag to true.
	
	When TickComponent() is called on the client again, if bUpdatePosition is true, the client will call
	ClientUpdatePosition() before calling PerformMovement().  ClientUpdatePosition() replays all the moves in the pending
	move list which occurred after the timestamp of the move the server was adjusting.
	*/

	/** Perform local movement and send the move to the server. */
	virtual void ReplicateMoveToServer(float DeltaTime, const FVector& NewAcceleration);

	/** If bUpdatePosition is true, then replay any unacked moves. Returns whether any moves were actually replayed. */
	virtual bool ClientUpdatePositionAfterServerUpdate();

	/** Call the appropriate replicated servermove() function to send a client player move to the server. */
	virtual void CallServerMove(class FSavedMove_Character* NewMove, const FVector& ClientLoc, uint8 ClientRoll, int32 View, class FSavedMove_Character* OldMove);
	
	/** Have the server check if the client is outside an error tolerance, and set a client adjustment if so. */
	void ServerMoveHandleClientError(float TimeStamp, float DeltaTime, const FVector& Accel, const FVector& ClientLoc);

	/* Process a move at the given time stamp, given the compressed flags representing various events that occurred (ie jump). */
	virtual void MoveAutonomous( float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAccel);

	/** Update character movement mode after a client adjustment. */
	virtual void UpdateMovementModeFromAdjustment();

public:
	/** Minimum time between client TimeStamp resets.
	 !! This has to be large enough so that we don't confuse the server if the client can stall or timeout.
	 We do this as we use floats for TimeStamps, and server derives DeltaTime from two TimeStamps. 
	 As time goes on, accuracy decreases from those floating point numbers.
	 So we trigger a TimeStamp reset at regular intervals to maintain a high level of accuracy. */
	UPROPERTY()
	float MinTimeBetweenTimeStampResets;

	/** On the Server, verify that an incoming client TimeStamp is valid and has not yet expired.
		It will also handle TimeStamp resets if it detects a gap larger than MinTimeBetweenTimeStampResets / 2.f
		!! ServerData.CurrentClientTimeStamp can be reset !!
		@returns true if TimeStamp is valid, or false if it has expired. */
	bool VerifyClientTimeStamp(float TimeStamp, FNetworkPredictionData_Server_Character & ServerData);

	// Callbacks for RPCs on Character
	virtual void ServerMove_Implementation(float TimeStamp, FVector_NetQuantize100 InAccel, FVector_NetQuantize100 ClientLoc, uint8 CompressedMoveFlags, uint8 ClientRoll, uint32 View);
	virtual void ServerMoveDual_Implementation(float TimeStamp0, FVector_NetQuantize100 InAccel0, uint8 PendingFlags, uint32 View0, float TimeStamp, FVector_NetQuantize100 InAccel, FVector_NetQuantize100 ClientLoc, uint8 NewFlags, uint8 ClientRoll, uint32 View);
	virtual void ServerMoveOld_Implementation(float OldTimeStamp, FVector_NetQuantize100 OldAccel, uint8 OldMoveFlags);
	virtual void ClientAckGoodMove_Implementation(float TimeStamp);
	virtual void ClientAdjustPosition_Implementation(float TimeStamp, FVector NewLoc, FVector NewVel, class UPrimitiveComponent* NewBase, bool bHasBase, bool bBaseRelativePosition);
	virtual void ClientVeryShortAdjustPosition_Implementation(float TimeStamp, FVector NewLoc, class UPrimitiveComponent* NewBase, bool bHasBase, bool bBaseRelativePosition);
	void ClientAdjustRootMotionPosition_Implementation(float TimeStamp, float ServerMontageTrackPosition, FVector ServerLoc, FVector_NetQuantizeNormal ServerRotation, float ServerVelZ, class UPrimitiveComponent * ServerBase, bool bHasBase, bool bBaseRelativePosition);

	// Root Motion
public:
	/** Root Motion movement params */
	UPROPERTY(Transient)
	FRootMotionMovementParams RootMotionParams;

	/** @return true if we have Root Motion to use in PerformMovement() physics. 
		Not valid outside of the scope of that function. Since RootMotion is extracted and used in it. */
	bool HasRootMotion() const
	{
		return RootMotionParams.bHasRootMotion;
	}

	/** Simulate Root Motion physics on Simulated Proxies */
	void SimulateRootMotion(float DeltaSeconds, const FTransform & LocalRootMotionTransform);

	// RVO Avoidance

	/** calculate RVO avoidance and apply it to current velocity */
	virtual void CalcAvoidanceVelocity(float DeltaTime);

	/** allows modifing avoidance velocity, called when bUseRVOPostProcess is set */
	virtual void PostProcessAvoidanceVelocity(FVector& NewVelocity);

protected:

	/** called in Tick to update data in RVO avoidance manager */
	void UpdateDefaultAvoidance();

	/** lock avoidance velocity */
	void SetAvoidanceVelocityLock(class UAvoidanceManager* Avoidance, float Duration);

public:

	/** Minimum acceptable distance for Character capsule to float above floor when walking. */
	static const float MIN_FLOOR_DIST;

	/** Maximum acceptable distance for Character capsule to float above floor when walking. */
	static const float MAX_FLOOR_DIST;

	/** Reject sweep impacts that are this close to the edge of the vertical portion of the capsule when performing vertical sweeps, and try again with a smaller capsule. */
	static const float SWEEP_EDGE_REJECT_DISTANCE;

	/** Stop completely when braking and velocity magnitude is lower than this. */
	static const float BRAKE_TO_STOP_VELOCITY;
};


typedef TSharedPtr<class FSavedMove_Character> FSavedMovePtr;

/** FSavedMove_Character represents a saved move on the client that has been sent to the server and might need to be played back. */
class ENGINE_API FSavedMove_Character
{
public:
	FSavedMove_Character();
	virtual ~FSavedMove_Character();

	uint32 bPressedJump:1;
	uint32 bWantsToCrouch:1;
	uint32 bForceMaxAccel:1;

	/** If true this move is using an old TimeStamp, before a reset occurred. */
	bool bOldTimeStampBeforeReset;

	float TimeStamp;    // Time of this move.
	float DeltaTime;    // amount of time for this move
	float CustomTimeDilation;

	FVector StartLocation;
	FVector StartRelativeLocation;
	FVector StartVelocity;
	FFindFloorResult StartFloor;
	FRotator StartRotation;
	FRotator StartControlRotation;
	FVector SavedLocation;
	FRotator SavedRotation;
	FVector SavedVelocity;
	FVector SavedRelativeLocation;
	FVector Acceleration;
	FRotator ControlRotation;

	// Cached to speed up iteration over IsImportantMove().
	FVector AccelNormal;
	float AccelMag;
	
	TWeakObjectPtr<UPrimitiveComponent> StartBase;
	TWeakObjectPtr<UPrimitiveComponent> EndBase;

	TWeakObjectPtr<class UAnimMontage> RootMotionMontage;
	float RootMotionTrackPosition;
	FRootMotionMovementParams RootMotionMovement;

	/** threshold for deciding this is an "important" move based on DP with last acked acceleration. */
	float AccelDotThreshold;    
	/** Threshold for deciding is this is an important move because acceleration magnitude has changed too much */
	float AccelMagThreshold;	
	
	/** Clear saved move properties, so it can be re-used. */
	virtual void Clear();

	/** Called to set up this saved move to make a predictive correction. */
	virtual void SetMoveFor(class ACharacter* C, float DeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character & ClientData);

	/** Set the properties describing the position, etc. of the moved pawn at the start of the move. */
	virtual void SetInitialPosition(class ACharacter* C);

	/** @Return true if this move is an "important" move that should be sent again if not acked by the server */
	virtual bool IsImportantMove(const FSavedMovePtr& LastAckedMove);
	
	/** Returns starting position of move, either absolute StartLocation, or StartRelativeLocation offset from StartBase. */
	virtual FVector GetStartLocation();

	/** Set the properties describing the final position, etc. of the moved pawn */
	virtual void PostUpdate(class ACharacter* C);
	
	/** @Return true if this move can be combined with NewMove for replication without changing any behavior */
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, class ACharacter* InPawn, float MaxDelta);
	
	/** Called before ClientUpdatePosition uses this SavedMove to make a predictive correction	 */
	virtual void PrepMoveFor( class ACharacter* C );

	/** Called after ClientUpdatePosition used this SavedMove to make a predictive correction	 */
	virtual void ResetMoveFor( class ACharacter* C );

	/** @returns a byte containing encoded special movement information (jumping, crouching, etc.)	 */
	virtual uint8 GetCompressedFlags();

	// Bit masks used by GetCompressedFlags() to encode movement information.
	enum CompressedFlags
	{
		FLAG_JumpPressed	= 0x01,	// Jump pressed
		FLAG_WantsToCrouch	= 0x02,	// Wants to crouch
		FLAG_Reserved_1		= 0x04,	// Reserved for future use
		FLAG_Reserved_2		= 0x08,	// Reserved for future use
		// Remaining bit masks are available for custom flags.
		FLAG_Custom_0		= 0x10,
		FLAG_Custom_1		= 0x20,
		FLAG_Custom_2		= 0x40,
		FLAG_Custom_3		= 0x80,
	};
};

// ClientAdjustPosition replication (event called at end of frame by server)
struct ENGINE_API FClientAdjustment
{
public:

	FClientAdjustment()
	: TimeStamp(0.f)
	, DeltaTime(0.f)
	, NewLoc(ForceInitToZero)
	, NewVel(ForceInitToZero)
	, NewRot(ForceInitToZero)
	, NewBase(NULL)
	, bAckGoodMove(0)
	, bBaseRelativePosition(false)
	{
	}

	float TimeStamp;
	float DeltaTime;
	FVector NewLoc;
	FVector NewVel;
	FRotator NewRot;
	class UPrimitiveComponent* NewBase;
	uint8 bAckGoodMove;
	bool bBaseRelativePosition;
};

class ENGINE_API FNetworkPredictionData_Client_Character : public FNetworkPredictionData_Client
{
public:

	FNetworkPredictionData_Client_Character();
	virtual ~FNetworkPredictionData_Client_Character();

	/** Client timestamp of last time it sent a servermove() to the server.  Used for holding off on sending movement updates to save bandwidth. */
	float ClientUpdateTime;

	/** Current TimeStamp for sending new Moves to the Server. */
	float CurrentTimeStamp;

	TArray<FSavedMovePtr> SavedMoves;		// Buffered moves pending position updates, orderd oldest to newest. Moves that have been acked by the server are removed.
	TArray<FSavedMovePtr> FreeMoves;		// freed moves, available for buffering
	FSavedMovePtr PendingMove;				// PendingMove already processed on client - waiting to combine with next movement to reduce client to server bandwidth
	FSavedMovePtr LastAckedMove;			// Last acknowledged sent move.

	int32 MaxFreeMoveCount;					// Limit on size of free list
	int32 MaxSavedMoveCount;					// Limit on the size of the saved move buffer

	uint32 bUpdatePosition:1; // when true, update the position (via ClientUpdatePosition)
	
	/** RootMotion saved while animation is updated, so we can store it and replay if needed in case of a position correction. */
	FRootMotionMovementParams RootMotionMovement;

	// Mesh smoothing variables (for network smoothing)
	//
	/** Whether to smoothly interpolate pawn position corrections on clients based on received location updates */
	uint32 bSmoothNetUpdates:1;

	/** Used for position smoothing in net games */
	FVector MeshTranslationOffset;

	/** Maximum location correction distance for which other pawn positions on a client will be smoothly updated */
	float MaxSmoothNetUpdateDist;

	/** If the updated location is more than NoSmoothNetUpdateDist from the current pawn position on the client, pop it to the updated location.
	If it is between MaxSmoothNetUpdateDist and NoSmoothNetUpdateDist, pop to MaxSmoothNetUpdateDist away from the updated location */
	float NoSmoothNetUpdateDist;

	/** How long to take to smoothly interpolate from the old pawn position on the client to the corrected one sent by the server.  Must be > 0.0 */
	float SmoothNetUpdateTime;
	
	// how long server will wait for client move update before setting position
	// @TODO: don't duplicate between server and client data (though it's used by both)
	float MaxResponseTime;

	/** Finds SavedMove index for given TimeStamp. Returns INDEX_NONE if not found (move has been already Acked or cleared). */
	int32 GetSavedMoveIndex(float TimeStamp) const;

	/** Ack a given move. This move will become LastAckedMove, SavedMoves will be adjusted to only contain unAcked moves. */
	void AckMove(int32 AckedMoveIndex);

	/** Allocate a new saved move. Subclasses should override this if they want to use a custom move class. */
	virtual FSavedMovePtr AllocateNewMove();

	/** Return a move to the free move pool. Assumes that 'Move' will no longer be referenced by anything but possibly the FreeMoves list. Clears PendingMove if 'Move' is PendingMove. */
	virtual void FreeMove(const FSavedMovePtr& Move);

	/** Tries to pull a pooled move off the free move list, otherwise allocates a new move. Returns NULL if the limit on saves moves is hit. */
	virtual FSavedMovePtr CreateSavedMove();

	/** Update CurentTimeStamp from passed in DeltaTime.
		It will measure the accuracy between passed in DeltaTime and how Server will calculate its DeltaTime.
		If inaccuracy is too high, it will reset CurrentTimeStamp to maintain a high level of accuracy.
		@return DeltaTime to use for Client's physics simulation prior to replicate move to server. */
	float UpdateTimeStampAndDeltaTime(float DeltaTime, class ACharacter & CharacterOwner, class UCharacterMovementComponent & CharacterMovementComponent);
};


class ENGINE_API FNetworkPredictionData_Server_Character : public FNetworkPredictionData_Server
{
public:

	FNetworkPredictionData_Server_Character();
	virtual ~FNetworkPredictionData_Server_Character();

	FClientAdjustment PendingAdjustment;

	float CurrentClientTimeStamp;	// Timestamp from the Client of most recent ServerMove() processed for this player
	float LastUpdateTime;			// Last time server updated client with a move correction or confirmation

	// how long server will wait for client move update before setting position
	// @TODO: don't duplicate between server and client data (though it's used by both)
	float MaxResponseTime;

	/** @return time delta to use for the current ServerMove() */
	float GetServerMoveDeltaTime(float TimeStamp) const;
};

