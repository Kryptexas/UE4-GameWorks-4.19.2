// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "EnginePrivate.h"
#include "MessageLog.h"

#define LOCTEXT_NAMESPACE "MovementComponent"
DEFINE_LOG_CATEGORY_STATIC(LogMovement, Log, All);

//----------------------------------------------------------------------//
// UMovementComponent
//----------------------------------------------------------------------//
UMovementComponent::UMovementComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	PrimaryComponentTick.bCanEverTick = true;

	MoveComponentFlags = MOVECOMP_NoFlags;

	bUpdateOnlyIfRendered = false;
	bAutoUpdateTickRegistration = true;
	bAutoRegisterUpdatedComponent = true;
	bConstrainToPlane = false;
	bSnapToPlaneAtStart = false;

	bWantsInitializeComponent = true;
	bAutoActivate = true;
	bInInitializeComponent = false;
}


void UMovementComponent::SetUpdatedComponent(UPrimitiveComponent* NewUpdatedComponent)
{
	if ( UpdatedComponent && UpdatedComponent!=NewUpdatedComponent )
	{
		UpdatedComponent->bShouldUpdatePhysicsVolume = false;
		UpdatedComponent->SetPhysicsVolume(NULL, true);
		UpdatedComponent->PhysicsVolumeChangedDelegate.Unbind();

		// remove from tick prerequisite
		UpdatedComponent->PrimaryComponentTick.RemovePrerequisite(this, PrimaryComponentTick); 
	}

	UpdatedComponent = NewUpdatedComponent;

	if ( UpdatedComponent != NULL )
	{
		UpdatedComponent->bShouldUpdatePhysicsVolume = true;
		UpdatedComponent->UpdatePhysicsVolume(true);
		UpdatedComponent->PhysicsVolumeChangedDelegate.BindUObject(this, &UMovementComponent::PhysicsVolumeChanged);
		
		// force ticks after movement component updates
		UpdatedComponent->PrimaryComponentTick.AddPrerequisite(this, PrimaryComponentTick); 
	}

	UpdateTickRegistration();

	if (bSnapToPlaneAtStart)
	{
		SnapUpdatedComponentToPlane();
	}
}


void UMovementComponent::InitializeComponent()
{
	bInInitializeComponent = true;
	Super::InitializeComponent();

	UPrimitiveComponent* NewUpdatedComponent = NULL;
	if (UpdatedComponent != NULL)
	{
		NewUpdatedComponent = UpdatedComponent;
	}
	else if (bAutoRegisterUpdatedComponent)
	{
		// Auto-register owner's root primitive component if found.
		AActor* MyActor = GetOwner();
		if (MyActor)
		{
			NewUpdatedComponent = MyActor->GetRootPrimitiveComponent();
			if (!NewUpdatedComponent)
			{
				FMessageLog("PIE").Warning(FText::Format(LOCTEXT("NoRootPrimitiveWarning", "Movement component {0} must update a PrimitiveComponent, but owning actor '{1}' does not have a root PrimitiveComponent. Auto registration failed."), 
					FText::FromString(GetName()),
					FText::FromString(MyActor->GetName())
					));
			}
		}
	}

	PlaneConstraintNormal = PlaneConstraintNormal.SafeNormal();
	SetUpdatedComponent(NewUpdatedComponent);
	bInInitializeComponent = false;
}

void UMovementComponent::RegisterComponentTickFunctions(bool bRegister)
{
	Super::RegisterComponentTickFunctions(bRegister);

	// If the owner ticks, make sure we tick first
	AActor* Owner = GetOwner();
	if (bRegister && Owner && Owner->CanEverTick())
	{
		Owner->PrimaryActorTick.AddPrerequisite(this, PrimaryComponentTick);
	}
}

void UMovementComponent::UpdateTickRegistration()
{
	if (!bAutoUpdateTickRegistration)
	{
		return;
	}

	const bool bAllowedToStartTick = (!bInInitializeComponent || bAutoActivate);
	const bool bHasUpdatedComponent = (UpdatedComponent != NULL);
	SetComponentTickEnabled(bHasUpdatedComponent && bAllowedToStartTick);
}


void UMovementComponent::PhysicsVolumeChanged(APhysicsVolume* NewVolume)
{
	//UE_LOG(LogMovement, Log, TEXT("PhysicsVolumeChanged : No implementation"));
}

APhysicsVolume* UMovementComponent::GetPhysicsVolume() const
{
	if (UpdatedComponent)
	{
		return UpdatedComponent->GetPhysicsVolume();
	}
	
	return GetWorld()->GetDefaultPhysicsVolume();
}

bool UMovementComponent::IsInWater() const
{
	return GetPhysicsVolume() && GetPhysicsVolume()->bWaterVolume;
}

bool UMovementComponent::SkipUpdate(float DeltaTime)
{
	return UpdatedComponent == NULL
			|| (bUpdateOnlyIfRendered 
				&& ((GetNetMode() == NM_DedicatedServer) || (GetWorld()->GetTimeSeconds() - UpdatedComponent->LastRenderTime > 0.2f)));
}


float UMovementComponent::GetGravityZ() const
{
	return GetPhysicsVolume()->GetGravityZ();
}

void UMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	
}

void UMovementComponent::UpdateComponentVelocity()
{
	if ( UpdatedComponent )
	{
		UpdatedComponent->ComponentVelocity = Velocity;
	}
}

void UMovementComponent::InitCollisionParams(FCollisionQueryParams &OutParams, FCollisionResponseParams& OutResponseParam) const
{
	if (UpdatedComponent)
	{
		UpdatedComponent->InitSweepCollisionParams(OutParams, OutResponseParam);
	}
}

bool UMovementComponent::OverlapTest(const FVector& Location, const FQuat& RotationQuat, const ECollisionChannel CollisionChannel, const FCollisionShape& CollisionShape, const AActor* IgnoreActor) const
{
	static FName NAME_TestOverlap = FName(TEXT("MovementOverlapTest"));
	FCollisionQueryParams QueryParams(NAME_TestOverlap, false, IgnoreActor);
	FCollisionResponseParams ResponseParam;
	InitCollisionParams(QueryParams, ResponseParam);
	return GetWorld()->OverlapTest(Location, RotationQuat, CollisionChannel, CollisionShape, QueryParams, ResponseParam);
}

bool UMovementComponent::IsExceedingMaxSpeed(float MaxSpeed) const
{
	MaxSpeed = FMath::Max(0.f, MaxSpeed);
	const float MaxSpeedSquared = FMath::Square(MaxSpeed);
	
	// Allow 1% error tolerance, to account for numeric imprecision.
	const float OverVelocityPercent = 1.01f;
	return (Velocity.SizeSquared() > MaxSpeedSquared * OverVelocityPercent);
}

void UMovementComponent::SetPlaneConstraintNormal(FVector PlaneNormal)
{
	PlaneConstraintNormal = PlaneNormal.SafeNormal();
}

void UMovementComponent::SetPlaneConstraintFromVectors(FVector Forward, FVector Up)
{
	PlaneConstraintNormal = (Up ^ Forward).SafeNormal();
}

void UMovementComponent::SetPlaneConstraintOrigin(FVector PlaneOrigin)
{
	PlaneConstraintOrigin = PlaneOrigin;
}

const FVector& UMovementComponent::GetPlaneConstraintOrigin() const
{
	return PlaneConstraintOrigin;
}

const FVector& UMovementComponent::GetPlaneConstraintNormal() const
{
	return PlaneConstraintNormal;
}

FVector UMovementComponent::ConstrainDirectionToPlane(FVector Direction) const
{
	if (bConstrainToPlane)
	{
		Direction = Direction - (PlaneConstraintNormal * (Direction | PlaneConstraintNormal));
	}

	return Direction;
}


FVector UMovementComponent::ConstrainLocationToPlane(FVector Location) const
{
	if (bConstrainToPlane)
	{
		Location = FVector::PointPlaneProject(Location, PlaneConstraintOrigin, PlaneConstraintNormal);
	}

	return Location;
}


void UMovementComponent::SnapUpdatedComponentToPlane()
{
	if (UpdatedComponent && bConstrainToPlane)
	{
		UpdatedComponent->SetWorldLocation( ConstrainLocationToPlane(UpdatedComponent->GetComponentLocation()) );
	}
}



bool UMovementComponent::MoveUpdatedComponent( const FVector& Delta, const FRotator& NewRotation, bool bSweep, FHitResult* OutHit)
{
	if (UpdatedComponent)
	{
		const FVector NewDelta = ConstrainDirectionToPlane(Delta);
		return UpdatedComponent->MoveComponent(NewDelta, NewRotation, bSweep, OutHit, MoveComponentFlags);
	}

	return false;
}


bool UMovementComponent::K2_MoveUpdatedComponent(FVector Delta, FRotator NewRotation, FHitResult& OutHit, bool bSweep)
{
	return SafeMoveUpdatedComponent(Delta, NewRotation, bSweep, OutHit);
}


bool UMovementComponent::SafeMoveUpdatedComponent(const FVector& Delta, const FRotator& NewRotation, bool bSweep, FHitResult& OutHit)
{
	if (UpdatedComponent == NULL)
	{
		OutHit.Reset(1.f);
		return false;
	}

	bool bMoveResult = MoveUpdatedComponent(Delta, NewRotation, bSweep, &OutHit);

	// Handle initial penetrations
	if (OutHit.bStartPenetrating && UpdatedComponent)
	{
		const FVector RequestedAdjustment = GetPenetrationAdjustment(OutHit);
		if (ResolvePenetration(RequestedAdjustment, OutHit, NewRotation))
		{
			// Retry original move
			bMoveResult = MoveUpdatedComponent(Delta, NewRotation, bSweep, &OutHit);
		}
	}

	return bMoveResult;
}


FVector UMovementComponent::GetPenetrationAdjustment(const FHitResult& Hit) const
{
	if (!Hit.bStartPenetrating)
	{
		return FVector::ZeroVector;
	}

	FVector Result;
	static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("p.PenetrationPullbackDistance"));
	const float PullBackDistance = (CVar ? FMath::Abs(CVar->GetValueOnGameThread()) : 0.125f);
	const float PenetrationDepth = (Hit.PenetrationDepth > 0.f ? Hit.PenetrationDepth : 0.125f);

	Result = Hit.Normal * (PenetrationDepth + PullBackDistance);

	return ConstrainDirectionToPlane(Result);
}


bool UMovementComponent::ResolvePenetration(const FVector& ProposedAdjustment, const FHitResult& Hit, const FRotator& NewRotation)
{
	const FVector Adjustment = ConstrainDirectionToPlane(ProposedAdjustment);
	if (!Adjustment.IsZero() && UpdatedComponent)
	{
		// See if we can fit at the adjusted location without overlapping anything.
		AActor* ActorOwner = UpdatedComponent->GetOwner();
		if (!ActorOwner)
		{
			return false;
		}

		// We really want to make sure that precision differences or differences between the overlap test and sweep tests don't put us into another overlap,
		// so make the overlap test a bit more restrictive.
		const float OverlapInflation = 0.10f;
		bool bEncroached = OverlapTest(Hit.TraceStart + Adjustment, NewRotation.Quaternion(), UpdatedComponent->GetCollisionObjectType(), UpdatedComponent->GetCollisionShape(OverlapInflation), ActorOwner);
		if (!bEncroached)
		{
			// Move without sweeping.
			MoveUpdatedComponent(Adjustment, NewRotation, false);
			UE_LOG(LogMovement, Verbose, TEXT("ResolvePenetration: teleport %s by %s"), *ActorOwner->GetName(), *Adjustment.ToString());
			return true;
		}
		else
		{
			// Try sweeping as far as possible...
			bool bMoved = MoveUpdatedComponent(Adjustment, NewRotation, true);
			UE_LOG(LogMovement, Verbose, TEXT("ResolvePenetration: sweep %s by %s (success = %d)"), *ActorOwner->GetName(), *Adjustment.ToString(), bMoved);
			
			if (!bMoved)
			{
				// Try moving the proposed adjustment plus the attempted move direction. This can sometimes get out of penetrations with multiple objects
				const FVector MoveDelta = (Hit.TraceEnd - Hit.TraceStart);
				if (!MoveDelta.IsZero())
				{
					bMoved = MoveUpdatedComponent(Adjustment + MoveDelta, NewRotation, true);
					UE_LOG(LogMovement, Verbose, TEXT("ResolvePenetration: sweep %s by %s (adjusted attempt success = %d)"), *ActorOwner->GetName(), *(Adjustment + MoveDelta).ToString(), bMoved);
				}
			}	

			return bMoved;
		}
	}

	return false;
}


FVector UMovementComponent::ComputeSlideVector(const FVector& Delta, const float Time, const FVector& Normal, const FHitResult& Hit) const
{
	return (Delta - Normal * (Delta | Normal)) * Time;
}


float UMovementComponent::SlideAlongSurface(const FVector& Delta, float Time, const FVector& Normal, FHitResult& Hit, bool bHandleImpact)
{
	float PercentTimeApplied = 0.f;
	const FVector OldHitNormal = Normal;

	FVector SlideDelta = ComputeSlideVector(Delta, Time, Normal, Hit);

	if ((SlideDelta | Delta) > 0.f)
	{
		const FRotator Rotation = UpdatedComponent->GetComponentRotation();
		SafeMoveUpdatedComponent(SlideDelta, Rotation, true, Hit);

		const float FirstHitPercent = Hit.Time;
		PercentTimeApplied = FirstHitPercent;
		if (Hit.IsValidBlockingHit())
		{
			// Notify first impact
			if (bHandleImpact)
			{
				HandleImpact(Hit, FirstHitPercent * Time, SlideDelta);
			}

			// Compute new slide normal when hitting multiple surfaces.
			TwoWallAdjust(SlideDelta, Hit, OldHitNormal);

			// Perform second move
			SafeMoveUpdatedComponent(SlideDelta, Rotation, true, Hit);
			const float SecondHitPercent = Hit.Time * (1.f - FirstHitPercent);
			PercentTimeApplied += SecondHitPercent;

			// Notify second impact
			if (bHandleImpact && Hit.bBlockingHit)
			{
				HandleImpact(Hit, SecondHitPercent * Time, SlideDelta);
			}
		}

		return FMath::Clamp(PercentTimeApplied, 0.f, 1.f);
	}

	return 0.f;
}


void UMovementComponent::TwoWallAdjust(FVector& OutDelta, const FHitResult& Hit, const FVector& OldHitNormal) const
{
	FVector Delta = OutDelta;
	const FVector HitNormal = Hit.Normal;

	if ((OldHitNormal | HitNormal) <= 0.f) //90 or less corner, so use cross product for direction
	{
		const FVector DesiredDir = Delta;
		FVector NewDir = (HitNormal ^ OldHitNormal);
		NewDir = NewDir.SafeNormal();
		Delta = (Delta | NewDir) * (1.f - Hit.Time) * NewDir;
		if ((DesiredDir | Delta) < 0.f)
		{
			Delta = -1.f * Delta;
		}
	}
	else //adjust to new wall
	{
		const FVector DesiredDir = Delta;
		Delta = (Delta - HitNormal * (Delta | HitNormal)) * (1.f - Hit.Time);
		if ((Delta | DesiredDir) <= 0.f)
		{
			Delta = FVector::ZeroVector;
		}
		else if ( FMath::Abs((HitNormal | OldHitNormal) - 1.f) < KINDA_SMALL_NUMBER )
		{
			// we hit the same wall again even after adjusting to move along it the first time
			// nudge away from it (this can happen due to precision issues)
			Delta += HitNormal * 0.1f;
		}
	}

	OutDelta = Delta;
}


#undef LOCTEXT_NAMESPACE
