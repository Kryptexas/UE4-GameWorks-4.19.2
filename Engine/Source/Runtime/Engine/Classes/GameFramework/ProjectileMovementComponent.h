// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "GameFramework/MovementComponent.h"
#include "ProjectileMovementComponent.generated.h"

/**
 * ProjectileMovementComponent updates the position of the associated UpdatedComponent during its tick.
 * Behavior such as bouncing after impacts and homing toward a target are supported.
 * If the updated component is simulating physics, only the initial launch parameters (when initial velocity is non-zero) will affect the projectile,
 * and the physics sim will take over from there.
 * @see UMovementComponent
 */
UCLASS(ClassGroup=Movement, meta=(BlueprintSpawnableComponent), ShowCategories=(Velocity))
class ENGINE_API UProjectileMovementComponent : public UMovementComponent
{
	GENERATED_UCLASS_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams( FOnProjectileBounceDelegate, const FHitResult&, ImpactResult, const FVector&, ImpactVelocity );
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FOnProjectileStopDelegate, const FHitResult&, ImpactResult );

	/** Initial speed of projectile. If greater than zero, this will override the initial Velocity value and instead treat Velocity as a direction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Projectile)
	float InitialSpeed;

	/** Limit on speed of projectile (0 means no limit). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Projectile)
	float MaxSpeed;

	/** If true, this projectile will have its rotation updated each frame to match its velocity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Projectile)
	uint32 bRotationFollowsVelocity:1;

	/** If true, simple bounces will be simulated.  Set to false to implement custom contact behavior (via HitActor) or to stop simulating on contact. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ProjectileBounces)
	uint32 bShouldBounce:1;

	/**
	 * If true, the initial Velocity is interpreted as being in local space upon startup.
	 * @see SetVelocityInLocalSpace()
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Projectile)
	uint32 bInitialVelocityInLocalSpace:1;

	/**
	 * If true, forces sub-stepping to break up movement into discrete smaller steps to improve accuracy of the trajectory.
	 * Objects that move in a straight line typically do *not* need to set this, as movement always uses continuous collision detection (sweeps) so collision is not missed.
	 * Sub-stepping is automatically enabled when under the effects of gravity or when homing towards a target.
	 * @see MaxSimulationTimeStep, MaxSimulationIterations
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ProjectileSimulation)
	uint32 bForceSubStepping:1;

	/**
	 * If true, we will accelerate towards our homing target.
	 * @see HomingTargetComponent, HomingAccelerationMagnitude
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Homing)
	uint32 bIsHomingProjectile:1;

	/** Custom gravity scale for this projectile.  Set to 0 for no gravity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Projectile)
	float ProjectileGravityScale;

	/** Buoyancy of UpdatedComponent in fluid.  0.0=sinks as fast as in air, 1.0=neutral buoyancy */
	UPROPERTY()
	float Buoyancy;

	/** Percentage of velocity maintained after the bounce in the direction of the normal of impact (coefficient of restitution). 1.0 = no velocity lost, 0.0 = no bounce. Ignored if bShouldBounce=false. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ProjectileBounces)
	float Bounciness;

	/** Percentage of velocity maintained after the bounce in the direction tangent to the normal of impact (coefficient of friction). 0.0 = no velocity lost, 1.0 = no sliding. Ignored if bShouldBounce=false. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ProjectileBounces)
	float Friction;

	/** If velocity is below this threshold after a bounce, stops simulating and triggers the OnProjectileStop event. Ignored if bShouldBounce=false, in which case the projectile stops simulating on the first impact. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ProjectileBounces)
	float BounceVelocityStopSimulatingThreshold;

	/** Called when projectile impacts something and bounces are enabled. */
	UPROPERTY(BlueprintAssignable)
	FOnProjectileBounceDelegate OnProjectileBounce;

	/** Called when projectile has come to a stop (velocity is below simulation threshold, bounces are disabled, or it is forcibly stopped). */
	UPROPERTY(BlueprintAssignable)
	FOnProjectileStopDelegate OnProjectileStop;

	/** The magnitude of our acceleration towards the homing target. Overall velocity magnitude will still be limited by MaxSpeed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Homing)
	float HomingAccelerationMagnitude;

	/**
	 * The current target we are homing towards. Can only be set at runtime (when projectile is spawned or updating).
	 * @see bIsHomingProjectile
	 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = Homing)
	TWeakObjectPtr<USceneComponent> HomingTargetComponent;

	/** Sets the velocity to the new value, rotated into Actor space. */
	UFUNCTION(BlueprintCallable, Category="Game|Components|ProjectileMovement")
	virtual void SetVelocityInLocalSpace(FVector NewVelocity);

	//Begin UActorComponent Interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual void Serialize( FArchive& Ar) override;
	//End UActorComponent Interface

	//Begin UMovementComponent Interface
	virtual float GetMaxSpeed() const override { return MaxSpeed; }
	virtual void InitializeComponent() override;
	//End UMovementComponent Interface

	/**
	 * This will check to see if the projectile is still in the world.  It will check things like
	 * the KillZ, outside world bounds, etc. and handle the situation.
	 */
	virtual bool CheckStillInWorld();

	/** @return Buoyancy of UpdatedComponent in fluid.  0.0=sinks as fast as in air, 1.0=neutral buoyancy*/
	float GetBuoyancy() const { return Buoyancy; };	

	bool ShouldApplyGravity() const { return ProjectileGravityScale != 0.f; }

	/** @returns the velocity after DeltaTime */
	virtual FVector CalculateVelocity(FVector OldVelocity, float DeltaTime, bool bGravityEnabled);

	/** Clears the reference to UpdatedComponent, fires stop event, and stops ticking. */
	UFUNCTION(BlueprintCallable, Category="Game|Components|ProjectileMovement")
	void StopSimulating(const FHitResult& HitResult);

	bool HasStoppedSimulation() { return UpdatedComponent == NULL; }

	/**
	 * Compute remaining time step given remaining time and current iterations.
	 * The last iteration (limited by MaxSimulationIterations) always returns the remaining time, which may violate MaxSimulationTimeStep.
	 *
	 * @param RemainingTime		Remaining time in the tick.
	 * @param Iterations		Current iteration of the tick (starting at 1).
	 * @return The remaining time step to use for the next sub-step of iteration.
	 * @see MaxSimulationTimeStep, MaxSimulationIterations
	 * @see ShouldUseSubStepping()
	 */
	float GetSimulationTimeStep(float RemainingTime, int32 Iterations) const;

	/**
	 * Determine whether or not to use substepping in the projectile motion update.
	 * If true, GetSimulationTimeStep() will be used to time-slice the update. If false, all remaining time will be used during the tick.
	 * @return Whether or not to use substepping in the projectile motion update.
	 * @see GetSimulationTimeStep()
	 */
	virtual bool ShouldUseSubStepping() const;

	/**
	 * Max time delta for each discrete simulation step. This is used if the object is under the effect of 
	 * Lowering this value can address issues with fast-moving objects or complex collision scenarios, at the cost of performance.
	 *
	 * WARNING: if (MaxSimulationTimeStep * MaxSimulationIterations) is too low for the min framerate, the last simulation step may exceed MaxSimulationTimeStep to complete the simulation.
	 * @see MaxSimulationIterations
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0166", ClampMax="0.50", UIMin="0.0166", UIMax="0.50"), Category=ProjectileSimulation)
	float MaxSimulationTimeStep;

	/**
	 * Max number of iterations used for each discrete simulation step
	 * Increasing this value can address issues with fast-moving objects or complex collision scenarios, at the cost of performance.
	 *
	 * WARNING: if (MaxSimulationTimeStep * MaxSimulationIterations) is too low for the min framerate, the last simulation step may exceed MaxSimulationTimeStep to complete the simulation.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, meta=(ClampMin="1", ClampMax="25", UIMin="1", UIMax="25"), Category=ProjectileSimulation)
	int32 MaxSimulationIterations;

protected:

	/** @return true if the wall hit has been fully processed, and no further movement is needed */
	bool HandleHitWall(const FHitResult& Hit, float TimeTick, const FVector& MoveDelta);

	/** Applies bounce logic (if enabled) to affect velocity upon impact, or stops the projectile if bounces are not enabled (or velocity is below threshold). Fires applicable events. */
	virtual void HandleImpact(const FHitResult& Hit, float TimeSlice=0.f, const FVector& MoveDelta = FVector::ZeroVector) override;

	/** Computes result of a bounce and returns the new velocity. */
	virtual FVector ComputeBounceResult(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta);

	/** Don't allow velocity magnitude to exceed MaxSpeed, if MaxSpeed is non-zero. */
	UFUNCTION(BlueprintCallable, Category="Game|Components|ProjectileMovement")
	FVector LimitVelocity(FVector NewVelocity) const;

	/** Compute the distance we should move in the given time, at a given a velocity. */
	virtual FVector ComputeMoveDelta(const FVector& InVelocity, float DeltaTime, bool bGravityEnabled = true) const;

	/** Compute the acceleration that will be applied */
	virtual FVector ComputeAcceleration(const FVector& InVelocity, float DeltaTime, bool bGravityEnabled) const;

	/** Allow the projectile to track towards its homing target. */
	virtual FVector ComputeHomingAcceleration(const FVector& InVelocity, float DeltaTime, bool bGravityEnabled) const;

	/** Compute gravity effect given current physics volume, projectile gravity scale, etc. */
	float GetEffectiveGravityZ() const;

	/** Minimum delta time considered when ticking. Delta times below this are not considered. This is a very small non-zero positive value to avoid potential divide-by-zero in simulation code. */
	static const float MIN_TICK_TIME;
};



