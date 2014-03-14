// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "ProjectileMovementComponent.generated.h"

/**
 * Projectile Movement component updates position of associated PrimitiveComponent during its tick.
 * If the updated component is simulating physics, only the initial launch parameters (when initial velocity is non-zero) will affect the projectile,
 * and the physics sim will take over from there.
 */
UCLASS(HeaderGroup=Component, ClassGroup=Movement, meta=(BlueprintSpawnableComponent), ShowCategories=(Velocity))
class ENGINE_API UProjectileMovementComponent : public UMovementComponent
{
	GENERATED_UCLASS_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FOnProjectileBounceDelegate, const FHitResult&, ImpactResult );
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

	/** Consider the projectile at rest if the velocity is below this threshold after a bounce (uu/sec). Ignored if bShouldBounce=false, in which case the projectile stops simulating on the first impact. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ProjectileBounces)
	float BounceVelocityStopSimulatingThreshold;

	/** Called when projectile impacts something and bounces are enabled. */
	UPROPERTY(BlueprintAssignable)
	FOnProjectileBounceDelegate OnProjectileBounce;

	/** Called when projectile has come to a stop (velocity is below simulation threshold, bounces are disabled, or it is forcibly stopped). */
	UPROPERTY(BlueprintAssignable)
	FOnProjectileStopDelegate OnProjectileStop;

	/** Sets the velocity to the new value, rotated into Actor space. */
	UFUNCTION(BlueprintCallable, Category="Game|Components|ProjectileMovement")
	virtual void SetVelocityInLocalSpace(FVector NewVelocity);

	//Begin UActorComponent Interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) OVERRIDE;
	virtual void Serialize( FArchive& Ar) OVERRIDE;
	//End UActorComponent Interface

	//Begin UMovementComponent Interface
	virtual float GetMaxSpeed() const OVERRIDE { return MaxSpeed; }
	virtual void InitializeComponent() OVERRIDE;
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
	virtual FVector CalculateVelocity(FVector OldVelocity, float DeltaTime);

	/** Clears the reference to UpdatedComponent, fires stop event, and stops ticking. */
	UFUNCTION(BlueprintCallable, Category="Game|Components|ProjectileMovement")
	void StopSimulating(const FHitResult& HitResult);

	bool HasStoppedSimulation() { return UpdatedComponent == NULL; }

protected:
	/** @return true if the wall hit has been fully processed, and no further movement is needed */
	bool HandleHitWall(const FHitResult& Hit, float TimeTick, const FVector& MoveDelta);

	/** Applies bounce logic (if enabled) to affect velocity upon impact, or stops the projectile if bounces are not enabled (or velocity is below threshold). Fires applicable events. */
	virtual void HandleImpact(const FHitResult& Hit, float TimeSlice=0.f, const FVector& MoveDelta = FVector::ZeroVector) OVERRIDE;

	/** Computes result of a bounce and returns the new velocity. */
	virtual FVector ComputeBounceResult(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta);

	// Don't allow velocity magnitude to exceed MaxSpeed, if MaxSpeed is non-zero.
	UFUNCTION(BlueprintCallable, Category="Game|Components|ProjectileMovement")
	FVector LimitVelocity(FVector NewVelocity) const;

	// Compute the distance we should move in the given time, at a given a velocity.
	virtual FVector ComputeMoveDelta(const FVector& InVelocity, float DeltaTime, bool bApplyGravity = true) const;

	// Compute gravity effect given current physics volume, projectile gravity scale, etc.
	float GetEffectiveGravityZ() const;
};



