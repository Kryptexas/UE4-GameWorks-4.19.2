// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Movement component updates position of associated PrimitiveComponent during its tick.
 */

#pragma once
#include "MovementComponent.generated.h"


UCLASS(HeaderGroup=Component, ClassGroup=Movement, abstract, BlueprintType)
class ENGINE_API UMovementComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	/** The component we move and update. **/
	UPROPERTY(BlueprintReadOnly, Category=MovementComponent)
	class UPrimitiveComponent* UpdatedComponent;

	/**
	 * Flags that control the behavior of calls to MoveComponent() on our UpdatedComponent.
	 * @see EMoveComponentFlags
	 */
	EMoveComponentFlags MoveComponentFlags;

	/** Current velocity of moved component, replicated to clients using Actor ReplicatedMovement property. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Velocity)
	FVector Velocity;

protected:
	/**
	 * The normal of the plane that constrains movement, if plane constraint is enabled.
	 * If for example you wanted to constrain movement to the Y-Z plane (so that X cannot change), the normal would be set to X=1 Y=0 Z=0.
	 * @see SetPlaneConstraintNormal(), SetPlaneConstraintFromVectors()
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=PlanarMovement)
	FVector PlaneConstraintNormal;

	/**
	 * The origin of the plane that constrains movement, if plane constraint is enabled. 
	 * This defines the behavior of snapping a position to the plane, such as by SnapUpdatedComponentToPlane().
	 * @see SetPlaneConstraintOrigin().
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=PlanarMovement)
	FVector PlaneConstraintOrigin;

public:
	/** If true, movement will be constrained to a plane.	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=PlanarMovement)
	uint32 bConstrainToPlane:1;

	/** If true, and if plane constraints are enabled, then the component updated by this MovementComponent will be snapped to the plane when first attached (in SetUpdatedComponent()).	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=PlanarMovement)
	uint32 bSnapToPlaneAtStart:1;


	/** If true, skips TickComponent() if UpdatedComponent was not recently rendered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=MovementComponent)
	uint32 bUpdateOnlyIfRendered:1;

	/**
	 * If true, whenever the updated component is changed, this component will enable or disable its tick dependent on whether it has something to update.
	 * This will NOT enable tick at startup if bAutoActivate is false, because presumably you have a good reason for not wanting it to start ticking initially.
	 **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=MovementComponent)
	uint32 bAutoUpdateTickRegistration:1;

	/** If true, registers the owner's Root component as the UpdatedComponent if there is not one currently assigned. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=MovementComponent)
	uint32 bAutoRegisterUpdatedComponent:1;

	// Begin ActorComponent interface 
	virtual void RegisterComponentTickFunctions(bool bRegister) OVERRIDE;

	/** Overridden to auto-register the updated component if it starts NULL, and we can find a root component on our owner. */
	virtual void InitializeComponent() OVERRIDE;
	// End ActorComponent interface

	/** @return gravity that affects this component */
	UFUNCTION(BlueprintCallable, Category="Components|Movement")
	virtual float GetGravityZ() const;

	/** @return maximum speed of component in current movement mode.  Multiply by MaxSpeedModifier for limitations based on posture, etc. */
	UFUNCTION(BlueprintCallable, Category="Components|Movement")
	virtual float GetMaxSpeed() const;

	/** @return a scalar applied to the maximum velocity that the component can currently move. */
	UFUNCTION(BlueprintCallable, Category="Components|Movement")
	virtual float GetMaxSpeedModifier() const;

	/** @return the result of GetMaxSpeed() * GetMaxSpeedModifier(). */
	UFUNCTION(BlueprintCallable, Category="Components|Movement")
	virtual float GetModifiedMaxSpeed() const;

	/**
	 * Returns true if the current velocity is exceeding the given max speed (usually the result of GetModifiedMaxSpeed()), within a small error tolerance.
	 * Note that under normal circumstances updates cause by acceleration will not cause this to be true, however external forces or changes in the max speed limit
	 * can cause the max speed to be violated.
	 */
	UFUNCTION(BlueprintCallable, Category="Components|Movement")
	virtual bool IsExceedingMaxSpeed(float MaxSpeed) const;

	/** Stops movement immediately (zeroes velocity, usually zeros acceleration for components with acceleration). */
	UFUNCTION(BlueprintCallable, Category="Components|Movement")
	virtual void StopMovementImmediately();

	/**
	 * Possibly skip update if moved component is not rendered.
	 * @param DeltaTime @todo this parameter is not used in the function.
	 * @return true if component movement update should be skipped
	 */
	virtual bool SkipUpdate(float DeltaTime);

	/** @return PhysicsVolume this MovementComponent is using, or the world's default physics volume if none. **/
	UFUNCTION(BlueprintCallable, Category="Components|Movement")
	virtual APhysicsVolume* GetPhysicsVolume() const;

	/** Delegate when PhysicsVolume of UpdatedComponent has been changed **/
	virtual void PhysicsVolumeChanged(class APhysicsVolume* NewVolume);

	/** add updated component to physics volume */
	UFUNCTION(BlueprintCallable, Category="Components|Movement")
	virtual void SetUpdatedComponent(class UPrimitiveComponent* NewUpdatedComponent);

	/** return true if it's in PhysicsVolume with water flag **/
	virtual bool IsInWater() const;

	/** Update tick registration state, determined by bAutoUpdateTickRegistration. Called by SetUpdatedComponent. */
	virtual void UpdateTickRegistration();

	/** 
	 * Called for Blocking impact
	 * @param Hit: Describes the collision.
	 * @param TimeSlice: Time period for the simulation that produced this hit.  Useful for
	 *		  putting Hit.Time in context.  Can be zero in certain situations where it's not appropriate, 
	 *		  be sure to handle that.
	 * @param MoveDelta: Attempted move that resulted in the hit.
	 */
	virtual void HandleImpact(const FHitResult& Hit, float TimeSlice=0.f, const FVector& MoveDelta = FVector::ZeroVector);

	/** Update ComponentVelocity of UpdatedComponent. This needs to be called by derived classes at the end of an update whenever Velocity has changed.	 */
	virtual void UpdateComponentVelocity();

	/** Initialize collision params appropriately based on our collision settings. Use this before any Line or Sweep tests. */
	virtual void InitCollisionParams(FCollisionQueryParams &OutParams, FCollisionResponseParams& OutResponseParam) const;

	/** Return true if the given collision shape overlaps other geometry at the given location and rotation. The collision params are set by InitCollisionParams(). */
	virtual bool OverlapTest(const FVector& Location, const FQuat& RotationQuat, const ECollisionChannel CollisionChannel, const FCollisionShape& CollisionShape, const AActor* IgnoreActor) const;

	/**
	 * Moves our UpdatedComponent by the given Delta, and sets rotation to NewRotation.
	 * Respects the plane constraint, if enabled.
	 */
	virtual bool MoveUpdatedComponent(const FVector& Delta, const FRotator& NewRotation, bool bSweep, FHitResult* OutHit = NULL);

	/**
	 * Moves our UpdatedComponent by the given Delta, and sets rotation to NewRotation.
	 * Respects the plane constraint, if enabled.
	 * @return True if some movement occurred, false if no movement occurred. Result of any impact will be stored in OutHit.
	 */
	UFUNCTION(BlueprintCallable, Category="Components|Movement", meta=(FriendlyName = "MoveUpdatedComponent"))
	bool K2_MoveUpdatedComponent(FVector Delta, FRotator NewRotation, FHitResult& OutHit, bool bSweep = true);

	/**
	 * Calls MoveUpdatedComponent(), handling initial penetrations by calling ResolvePenetration().
	 * If this adjustment succeeds, the original movement will be attempted again.
	 * @return result of the final MoveUpdatedComponent() call.
	 */
	bool SafeMoveUpdatedComponent(const FVector& Delta, const FRotator& NewRotation, bool bSweep, FHitResult& OutHit);

	/**
	 * Calculate a movement adjustment to try to move out of a penetration from a failed move.
	 * @param Hit - the result of the failed move
	 * @return The adjustment to use after a failed move, or a zero vector if no attempt should be made.
	 */
	virtual FVector GetPenetrationAdjustment(const FHitResult& Hit) const;
	
	/**
	 * Try to move out of penetration in an object after a failed move.
	 * This function should respect the plane constraint if applicable.
	 * @param Adjustment - the requested adjustment, usually from GetPenetrationAdjustment().
	 * @param Hit - the result of the failed move
	 * @return true if the adjustment was successful and the original move should be retried, or false if no repeated attempt should be made.
	 */
	virtual bool ResolvePenetration(const FVector& Adjustment, const FHitResult& Hit, const FRotator& NewRotation);

	/**
	 * Compute a vector to slide along a surface, given an attempted move, time, and normal.
	 * @param Delta:	Attempted move.
	 * @param Time:		Amount of move to apply, usually between 0 and 1.
	 * @param Normal:	Normal opposed to movement. May not equal Hit.Normal.
	 * @param Hit:		Hit result of the move that resulted in the slide.
	 */
	virtual FVector ComputeSlideVector(const FVector& Delta, const float Time, const FVector& Normal, const FHitResult& Hit) const;

	/**
	 * Slide smoothly along a surface, and slide away from multiple impacts using TwoWallAdjust if necessary. Calls HandleImpact for the first surface hit, if requested.
	 * Uses SafeMoveUpdatedComponent for movement, and ComputeSlideVector to determine the slide direction.
	 * @param bHandleImpact - Whether to call HandleImpact on each hit.
	 * @return The percent of the Time value actually applied to movement (between 0 and 1). 0 if no movement occurred, non-zero if movement occurred (in which case Hit.Time is the result of the last move).
	 */
	virtual float SlideAlongSurface(const FVector& Delta, float Time, const FVector& Normal, FHitResult &Hit, bool bHandleImpact = false);

	/**
	 * Compute a movement direction when contacting two surfaces.
	 * @param Delta:		[In] Amount of move attempted before impact. [Out] Computed adjustment based on impacts.
	 * @param Hit:			Impact from last attempted move
	 * @param OldHitNormal:	Normal of impact before last attempted move
	 * @return Result in Delta that is the direction to move when contacting two surfaces.
	 */
	virtual void TwoWallAdjust(FVector &Delta, const FHitResult& Hit, const FVector &OldHitNormal) const;

	/**
	 * Sets the normal of the plane that constrains movement, if plane constraint is enabled.
	 * @param PlaneNormal	- The normal of the plane. If non-zero, it will be normalized.
	 */
	UFUNCTION(BlueprintCallable, Category="Components|Movement|Planar")
	virtual void SetPlaneConstraintNormal(FVector PlaneNormal);

	/** Uses the Forward and Up vectors to compute the plane that constrains movement, if plane constraint is enabled.	 */
	UFUNCTION(BlueprintCallable, Category="Components|Movement|Planar")
	virtual void SetPlaneConstraintFromVectors(FVector Forward, FVector Up);

	/** Sets the origin of the plane that constrains movement, if plane constraint is enabled.	 */
	UFUNCTION(BlueprintCallable, Category="Components|Movement|Planar")
	virtual void SetPlaneConstraintOrigin(FVector PlaneOrigin);

	/** @return The normal of the plane that constrains movement, if plane constraint is enabled.	 */
	UFUNCTION(BlueprintCallable, Category="Components|Movement|Planar")
	const FVector& GetPlaneConstraintNormal() const;

	/**
	 * @return The origin of the plane that constrains movement, if plane constraint is enabled.
	 * This defines the behavior of snapping a position to the plane, such as by SnapUpdatedComponentToPlane().
	 */
	UFUNCTION(BlueprintCallable, Category="Components|Movement|Planar")
	const FVector& GetPlaneConstraintOrigin() const;

	/** Constrain a direction vector to the plane constraint, if enabled.	 */
	UFUNCTION(BlueprintCallable, Category="Components|Movement|Planar")
	virtual FVector ConstrainDirectionToPlane(FVector Direction) const;

	/** Constrain a position vector to the plane constraint, if enabled.	 */
	UFUNCTION(BlueprintCallable, Category="Components|Movement|Planar")
	virtual FVector ConstrainLocationToPlane(FVector Location) const;

	/** Snap the updated component to the plane constraint, if enabled.	 */
	UFUNCTION(BlueprintCallable, Category="Components|Movement|Planar")
	virtual void SnapUpdatedComponentToPlane();

private:
	/** Transient flag indicating whether we are executing InitializeComponent. */
	uint32 bInInitializeComponent:1;
};


//////////////////////////////////////////////////////////////////////////
// Inlines

inline float UMovementComponent::GetMaxSpeed() const
{
	return 0.f;
}

inline float UMovementComponent::GetMaxSpeedModifier() const
{
	return 1.0f;
}

inline float UMovementComponent::GetModifiedMaxSpeed() const
{
	return GetMaxSpeed() * GetMaxSpeedModifier();
}

inline void UMovementComponent::StopMovementImmediately()
{
	Velocity = FVector::ZeroVector;
}
