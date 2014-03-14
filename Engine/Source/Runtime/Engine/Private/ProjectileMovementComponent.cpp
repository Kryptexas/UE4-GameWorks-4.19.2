// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

DEFINE_LOG_CATEGORY_STATIC(LogProjectileMovement, Log, All);

UProjectileMovementComponent::UProjectileMovementComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bUpdateOnlyIfRendered = false;
	bInitialVelocityInLocalSpace = true;

	Velocity = FVector(1.f,0.f,0.f);

	ProjectileGravityScale = 1.f;

	Bounciness = 0.6f;
	Friction = 0.2f;
	BounceVelocityStopSimulatingThreshold = 5.f;
	bWantsInitializeComponent = true;
}

void UProjectileMovementComponent::Serialize( FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_REFACTOR_PROJECTILE_MOVEMENT)
	{
		// Old code used to treat Bounciness as Friction as well.
		Friction = FMath::Clamp(1.f - Bounciness, 0.f, 1.f);

		// Old projectiles probably don't want to use this behavior by default.
		bInitialVelocityInLocalSpace = false;
	}
}


void UProjectileMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (Velocity.SizeSquared() > 0.f)
	{
		// InitialSpeed > 0 overrides initial velocity magnitude.
		if (InitialSpeed > 0.f)
		{
			Velocity = Velocity.SafeNormal() * InitialSpeed;
		}

		if (bInitialVelocityInLocalSpace)
		{
			SetVelocityInLocalSpace(Velocity);
		}

		if (bRotationFollowsVelocity)
		{
			if (UpdatedComponent)
			{
				UpdatedComponent->SetWorldRotation(Velocity.Rotation());
			}
		}

		UpdateComponentVelocity();
		
		if (UpdatedComponent && UpdatedComponent->IsSimulatingPhysics())
		{
			UpdatedComponent->SetPhysicsLinearVelocity(Velocity);
		}
	}
}


void UProjectileMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	QUICK_SCOPE_CYCLE_COUNTER( STAT_ProjectileMovementComponent_TickComponent );

	// skip if don't want component updated when not rendered
	if ( SkipUpdate(DeltaTime) )
	{
		return;
	}
	AActor* ActorOwner = UpdatedComponent->GetOwner();
	if ( !ActorOwner || !CheckStillInWorld() )
	{
		return;
	}

	if (UpdatedComponent->IsSimulatingPhysics())
	{
		return;
	}

	float RemainingTime	= DeltaTime;
	int32	NumBounces = 0;
	int32 Iterations = 0;
	FHitResult Hit(1.f);
	FVector OldHitNormal(0.f,0.f,1.f);
	bool bSliding = false;

	while( RemainingTime > 0.f && (Iterations < 8) && !ActorOwner->IsPendingKill() && UpdatedComponent )
	{
		Iterations++;

		// subdivide long ticks if falling to more closely follow parabolic trajectory
		float TimeTick = (!ShouldApplyGravity() || RemainingTime < 0.05f) ? RemainingTime : FMath::Min(0.05f, RemainingTime*0.5f);
		RemainingTime -= TimeTick;
		Hit.Time = 1.f;

		const FVector OldVelocity = Velocity;
		const bool bApplyGravity = !bSliding;
		FVector MoveDelta = ComputeMoveDelta(Velocity, TimeTick, bApplyGravity);

		const FVector TmpVelocity = Velocity;
		const FRotator NewRotation = bRotationFollowsVelocity ? Velocity.Rotation() : ActorOwner->GetActorRotation();
		SafeMoveUpdatedComponent( MoveDelta, NewRotation, true, Hit );
		if( ActorOwner->IsPendingKill() || !UpdatedComponent )
		{
			return;
		}

		if (Hit.Time == 1.f)
		{
			bSliding = false;
			Velocity = CalculateVelocity(Velocity, TimeTick);
		}
		else
		{
			if ( Velocity == TmpVelocity )
			{
				// re-calculate end velocity for partial time
				Velocity = CalculateVelocity(OldVelocity, TimeTick*Hit.Time);
			}
			if ( HandleHitWall(Hit, TimeTick*Hit.Time, MoveDelta) )
			{
				return;
			}
			if( NumBounces < 2 )
			{
				RemainingTime += TimeTick * (1.f - Hit.Time);
			}
			NumBounces++;

			// if velocity still into wall (after HitWall() had a chance to adjust), slide along wall
			if ( (Velocity | Hit.Normal) <= 0.f || (Hit.Time == 0.f && NumBounces > 1) )
			{
				if ( (NumBounces > 1) && ((OldHitNormal | Hit.Normal) <= 0.f) )
				{
					//90 degree or less corner, so use cross product for direction
					FVector NewDir = (Hit.Normal ^ OldHitNormal);
					NewDir = NewDir.SafeNormal();
					Velocity = (Velocity | NewDir) * NewDir;
					if ((TmpVelocity | Velocity) < 0.f)
					{
						Velocity *= -1.f;
					}
				}
				else 
				{
					//adjust to move along new wall
					Velocity = Velocity - Hit.Normal * (Velocity | Hit.Normal);
					if ( (Velocity | OldVelocity) <= 0.f )
					{
						if (Velocity.SizeSquared() < FMath::Square(BounceVelocityStopSimulatingThreshold))
						{
							StopSimulating(Hit);
							break;
						}
					}
				}
				OldHitNormal = Hit.Normal;
				bSliding = true;
			}
		}
	}

	UpdateComponentVelocity();
}


void UProjectileMovementComponent::SetVelocityInLocalSpace(FVector NewVelocity)
{
	if (UpdatedComponent)
	{
		Velocity = UpdatedComponent->GetComponentToWorld().TransformVectorNoScale(NewVelocity);
	}
}


FVector UProjectileMovementComponent::CalculateVelocity(FVector OldVelocity, float DeltaTime)
{
	FVector NewVelocity = OldVelocity;

	if ( ShouldApplyGravity() )
	{
		NewVelocity.Z += GetEffectiveGravityZ() * DeltaTime;
	}

	return LimitVelocity(NewVelocity);
}


FVector UProjectileMovementComponent::LimitVelocity(FVector NewVelocity) const
{
	if (GetMaxSpeed() > 0.f)
	{
		NewVelocity = NewVelocity.ClampMaxSize(GetModifiedMaxSpeed());
	}

	return NewVelocity;
}


FVector UProjectileMovementComponent::ComputeMoveDelta(const FVector& InVelocity, float DeltaTime, bool bApplyGravity) const
{
	// x = x0 + v*t
	FVector Delta = InVelocity * DeltaTime;

	// y = y0 + v*t + 1/2*a*t^2
	if (bApplyGravity)
	{
		Delta.Z += 0.5f * GetEffectiveGravityZ() * FMath::Square(DeltaTime);
	}

	return Delta;
}


float UProjectileMovementComponent::GetEffectiveGravityZ() const
{
	// TODO: apply buoyancy if in water
	return GetGravityZ() * ProjectileGravityScale;
}


void UProjectileMovementComponent::StopSimulating(const FHitResult& HitResult)
{
	SetUpdatedComponent(NULL);
	Velocity = FVector::ZeroVector;
	OnProjectileStop.Broadcast(HitResult);
}


bool UProjectileMovementComponent::HandleHitWall(const FHitResult& Hit, float TimeTick, const FVector& MoveDelta)
{
	AActor* ActorOwner = UpdatedComponent ? UpdatedComponent->GetOwner() : NULL;
	if ( !CheckStillInWorld() || !ActorOwner || ActorOwner->IsPendingKill() )
	{
		return true;
	}

	HandleImpact(Hit, TimeTick, MoveDelta);
	
	if( ActorOwner->IsPendingKill() || !UpdatedComponent )
	{
		return true;
	}

	return false;
}

FVector UProjectileMovementComponent::ComputeBounceResult(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	FVector TempVelocity = Velocity;

	// Project velocity onto normal in reflected direction.
	const FVector ProjectedNormal = Hit.Normal * -(TempVelocity | Hit.Normal);

	// Point velocity in direction parallel to surface
	TempVelocity += ProjectedNormal;

	// Only tangential velocity should be affected by friction.
	TempVelocity *= FMath::Clamp(1.f - Friction, 0.f, 1.f);

	// Coefficient of restitution only applies perpendicular to impact.
	TempVelocity += (ProjectedNormal * FMath::Max(Bounciness, 0.f));

	// Bounciness or Friction > 1 could cause us to exceed velocity.
	TempVelocity = LimitVelocity(TempVelocity);

	return TempVelocity;
}

void UProjectileMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	bool bStopSimulating = false;

	if (bShouldBounce)
	{
		Velocity = ComputeBounceResult(Hit, TimeSlice, MoveDelta);

		if (Velocity.SizeSquared() < FMath::Square(BounceVelocityStopSimulatingThreshold))
		{
			bStopSimulating = true;
		}

		// Trigger bounce events
		OnProjectileBounce.Broadcast(Hit);
	}
	else
	{
		bStopSimulating = true;
	}


	if (bStopSimulating)
	{
		StopSimulating(Hit);
	}
}

bool UProjectileMovementComponent::CheckStillInWorld()
{
	if ( !UpdatedComponent )
	{
		return false;
	}
	// check the variations of KillZ
	AWorldSettings* WorldSettings = GetWorld()->GetWorldSettings( true );
	if (!WorldSettings->bEnableWorldBoundsChecks)
	{
		return true;
	}
	AActor* ActorOwner = UpdatedComponent->GetOwner();
	if ( !ActorOwner )
	{
		return false;
	}
	if( ActorOwner->GetActorLocation().Z < WorldSettings->KillZ )
	{
		UDamageType const* DmgType = WorldSettings->KillZDamageType ? WorldSettings->KillZDamageType->GetDefaultObject<UDamageType>() : GetDefault<UDamageType>();
		ActorOwner->FellOutOfWorld(*DmgType);
		return false;
	}
	// Check if box has poked outside the world
	else if( UpdatedComponent && UpdatedComponent->IsRegistered() )
	{
		const FBox&	Box = UpdatedComponent->Bounds.GetBox();
		if(	Box.Min.X < -HALF_WORLD_MAX || Box.Max.X > HALF_WORLD_MAX ||
			Box.Min.Y < -HALF_WORLD_MAX || Box.Max.Y > HALF_WORLD_MAX ||
			Box.Min.Z < -HALF_WORLD_MAX || Box.Max.Z > HALF_WORLD_MAX )
		{
			UE_LOG(LogProjectileMovement, Warning, TEXT("%s is outside the world bounds!"), *ActorOwner->GetName());
			ActorOwner->OutsideWorldBounds();
			// not safe to use physics or collision at this point
			ActorOwner->SetActorEnableCollision(false);
			FHitResult Hit;
			StopSimulating(Hit);
			return false;
		}
	}
	return true;
}
