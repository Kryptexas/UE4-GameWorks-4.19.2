// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Movement.cpp: Character movement implementation

=============================================================================*/

#include "EnginePrivate.h"
#include "EngineInterpolationClasses.h"

DEFINE_LOG_CATEGORY_STATIC(LogCharacterMovement, Log, All);

// MAGIC NUMBERS
const float MAX_STEP_SIDE_Z = 0.08f;	// maximum z value for the normal on the vertical side of steps
const float SWIMBOBSPEED = -80.f;

const float UCharacterMovementComponent::MIN_FLOOR_DIST = 1.9f;
const float UCharacterMovementComponent::MAX_FLOOR_DIST = 2.4f;
const float UCharacterMovementComponent::BRAKE_TO_STOP_VELOCITY = 10.f;
const float UCharacterMovementComponent::SWEEP_EDGE_REJECT_DISTANCE = 0.15f;


// Version that does not use inverse sqrt estimate, for higher precision.
FORCEINLINE FVector SafeNormalPrecise(const FVector& V)
{
	const float VSq = V.SizeSquared();
	if (VSq < SMALL_NUMBER)
	{
		return FVector::ZeroVector;
	}
	else
	{
		return V * (1.f / FMath::Sqrt(VSq));
	}
}

// Version that does not use inverse sqrt estimate, for higher precision.
FORCEINLINE FVector ClampMaxSizePrecise(const FVector& V, float MaxSize)
{
	if (MaxSize < KINDA_SMALL_NUMBER)
	{
		return FVector::ZeroVector;
	}

	const float VSq = V.SizeSquared();
	if (VSq > FMath::Square(MaxSize))
	{
		return V * (MaxSize / FMath::Sqrt(VSq));
	}
	else
	{
		return V;
	}
}


void FFindFloorResult::SetFromSweep(const FHitResult& InHit, const float InSweepFloorDist, const bool bIsWalkableFloor)
{
	bBlockingHit = InHit.IsValidBlockingHit();
	bWalkableFloor = bIsWalkableFloor;
	bLineTrace = false;
	FloorDist = InSweepFloorDist;
	LineDist = 0.f;
	HitResult = InHit;
}

void FFindFloorResult::SetFromLineTrace(const FHitResult& InHit, const float InSweepFloorDist, const float InLineDist, const bool bIsWalkableFloor)
{
	// We require a sweep that hit if we are going to use a line result.
	check(HitResult.bBlockingHit);
	if (HitResult.bBlockingHit && InHit.bBlockingHit)
	{
		// Override most of the sweep result with the line result, but save some values
		FHitResult OldHit(HitResult);
		HitResult = InHit;

		// Restore some of the old values. We want the new normals and hit actor, however.
		HitResult.Time = OldHit.Time;
		HitResult.ImpactPoint = OldHit.ImpactPoint;
		HitResult.Location = OldHit.Location;
		HitResult.TraceStart = OldHit.TraceStart;
		HitResult.TraceEnd = OldHit.TraceEnd;

		bLineTrace = true;
		LineDist = InLineDist;
		bWalkableFloor = bIsWalkableFloor;
	}
}


UCharacterMovementComponent::UCharacterMovementComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	GravityScale = 1.f;
	GroundFriction = 8.0f;
	JumpZVelocity = 420.0f;
	JumpOffJumpZFactor = 0.5f;
	RotationRate = FRotator(0.f, 360.0f, 0.0f);
	SetWalkableFloorZ(0.71f);

	MaxStepHeight = 45.0f;
	PerchRadiusThreshold = 0.0f;
	PerchAdditionalHeight = 40.f;

	MaxFlySpeed = 600.0f;
	MaxWalkSpeed = 600.0f;
	MaxSwimSpeed = 300.0f;
	
	CrouchedSpeedMultiplier = 0.5f;
	MaxOutOfWaterStepHeight = 40.0f;
	OutofWaterZ = 420.0f;
	AirControl = 0.05f;
	FallingLateralFriction = 0.f;
	MaxAcceleration = 2048.0f;
	BrakingDecelerationWalking = MaxAcceleration;
	BrakingDecelerationFalling = 0.f;
	BrakingDecelerationFlying = 0.f;
	BrakingDecelerationSwimming = 0.f;
	LedgeCheckThreshold = 4.0f;
	JumpOutOfWaterPitch = 11.25f;
	UpperImpactNormalScale = 0.5f;
	Mass = 100.0f;
	bJustTeleported = true;
	CrouchedHalfHeight = 40.0f;
	PendingLaunchVelocity = FVector::ZeroVector;
	DefaultWaterMovementMode = MOVE_Swimming;
	DefaultLandMovementMode = MOVE_Falling;
	bForceBraking_DEPRECATED = false;
	bShrinkProxyCapsule = true;
	bCanWalkOffLedges = true;
	bCanWalkOffLedgesWhenCrouching = false;

	bEnablePhysicsInteraction = true;
	InitialPushForceFactor = 500.0f;;
	PushForceFactor = 750000.0f;
	PushForcePointZOffsetFactor = -0.75f;
	bPushForceScaledToMass = false;
	bScalePushForceToVelocity = true;
	
	TouchForceFactor = 1.0f;
	bTouchForceScaledToMass = true;
	MinTouchForce = -1.0f;
	MaxTouchForce = 250.0f;
	RepulsionForce = 2.5f;

	bUseControllerDesiredRotation = false;

	bMaintainHorizontalGroundVelocity = true;
	bImpartBaseVelocityX = true;
	bImpartBaseVelocityY = true;
	bImpartBaseVelocityZ = true;
	bImpartBaseAngularVelocity = true;

	// default character can jump and walk
	NavAgentProps.bCanJump = true;
	NavAgentProps.bCanWalk = true;
	ResetMoveState();

	ClientPredictionData = NULL;
	ServerPredictionData = NULL;

	// This should be greater than tolerated player timeout * 2.
	MinTimeBetweenTimeStampResets = 4.f * 60.f; 

	bEnableScopedMovementUpdates = true;

	bRequestedMoveUseAcceleration = true;
	bUseRVOAvoidance = false;
	bUseRVOPostProcess = false;
	AvoidanceLockTimer = 0.0f;
	AvoidanceGroup.bGroup0 = true;
	GroupsToAvoid.Packed = 0xFFFFFFFF;
	GroupsToIgnore.Packed = 0;
}

void UCharacterMovementComponent::PostLoad()
{
	Super::PostLoad();

	const int32 LinkerUE4Ver = GetLinkerUE4Version();

	if (LinkerUE4Ver < VER_UE4_CHARACTER_MOVEMENT_DECELERATION)
	{
		BrakingDecelerationWalking = MaxAcceleration;
	}

	if (LinkerUE4Ver < VER_UE4_CHARACTER_BRAKING_REFACTOR)
	{
		// This bool used to apply walking braking in flying and swimming modes.
		if (bForceBraking_DEPRECATED)
		{
			BrakingDecelerationFlying = BrakingDecelerationWalking;
			BrakingDecelerationSwimming = BrakingDecelerationWalking;
		}
	}

	if (LinkerUE4Ver < VER_UE4_CHARACTER_MOVEMENT_WALKABLE_FLOOR_REFACTOR)
	{
		// Compute the walkable floor angle, since we have never done so yet.
		UCharacterMovementComponent::SetWalkableFloorZ(WalkableFloorZ);
	}
}


#if WITH_EDITOR
void UCharacterMovementComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const UProperty* PropertyThatChanged = PropertyChangedEvent.MemberProperty;
	if (PropertyThatChanged && PropertyThatChanged->GetFName() == GET_MEMBER_NAME_CHECKED(UCharacterMovementComponent, WalkableFloorAngle))
	{
		// Compute WalkableFloorZ from the Angle.
		SetWalkableFloorAngle(WalkableFloorAngle);
	}
}
#endif // WITH_EDITOR


void UCharacterMovementComponent::OnUnregister()
{
	delete ClientPredictionData;
	ClientPredictionData = NULL;

	delete ServerPredictionData;
	ServerPredictionData = NULL;

	Super::OnUnregister();
}

void UCharacterMovementComponent::SetUpdatedComponent(UPrimitiveComponent* NewUpdatedComponent)
{
	if (NewUpdatedComponent)
	{
		const ACharacter* NewCharacterOwner = Cast<ACharacter>(NewUpdatedComponent->GetOwner());
		if (NewCharacterOwner == NULL)
		{
			UE_LOG(LogCharacterMovement, Error, TEXT("%s owned by %s must update a component owned by a Character"), *GetName(), *GetNameSafe(NewUpdatedComponent->GetOwner()));
			return;
		}

		// check that UpdatedComponent is a Capsule
		if (Cast<UCapsuleComponent>(NewUpdatedComponent) == NULL)
		{
			UE_LOG(LogCharacterMovement, Error, TEXT("%s owned by %s must update a capsule component"), *GetName(), *GetNameSafe(NewUpdatedComponent->GetOwner()));
			return;
		}
	}

	if ( bMovementInProgress )
	{
		// failsafe to avoid crashes in CharacterMovement. 
		bDeferUpdateMoveComponent = true;
		DeferredUpdatedMoveComponent = NewUpdatedComponent;
		return;
	}
	bDeferUpdateMoveComponent = false;
	DeferredUpdatedMoveComponent = NULL;

	if (UpdatedComponent != NULL && UpdatedComponent->OnComponentBeginOverlap.IsBound() && bEnablePhysicsInteraction)
	{
		UpdatedComponent->OnComponentBeginOverlap.RemoveDynamic(this, &UCharacterMovementComponent::CapsuleTouched);
	}
	
	Super::SetUpdatedComponent(NewUpdatedComponent);
	CharacterOwner = Cast<ACharacter>(PawnOwner);

	if (bEnablePhysicsInteraction)
	{
		UpdatedComponent->OnComponentBeginOverlap.AddDynamic(this, &UCharacterMovementComponent::CapsuleTouched);
	}

	if (bUseRVOAvoidance)
	{
		UAvoidanceManager* AvoidanceManager = GetWorld()->GetAvoidanceManager();
		if (AvoidanceManager)
		{
			AvoidanceManager->RegisterMovementComponent(this, AvoidanceWeight);
		}
	}
}


ACharacter* UCharacterMovementComponent::GetCharacterOwner() const
{
	return CharacterOwner;
}


FCollisionShape UCharacterMovementComponent::GetPawnCapsuleCollisionShape(const EShrinkCapsuleExtent ShrinkMode, const float CustomShrinkAmount) const
{
	FVector Extent = GetPawnCapsuleExtent(ShrinkMode, CustomShrinkAmount);
	return FCollisionShape::MakeCapsule(Extent);
}

FVector UCharacterMovementComponent::GetPawnCapsuleExtent(const EShrinkCapsuleExtent ShrinkMode, const float CustomShrinkAmount) const
{
	check(CharacterOwner);

	float Radius, HalfHeight;
	CharacterOwner->CapsuleComponent->GetScaledCapsuleSize(Radius, HalfHeight);
	FVector CapsuleExtent(Radius, Radius, HalfHeight);

	float RadiusEpsilon = 0.f;
	float HeightEpsilon = 0.f;

	switch(ShrinkMode)
	{
	case SHRINK_None:
		return CapsuleExtent;

	case SHRINK_RadiusCustom:
		RadiusEpsilon = CustomShrinkAmount;
		break;

	case SHRINK_HeightCustom:
		HeightEpsilon = CustomShrinkAmount;
		break;
		
	case SHRINK_AllCustom:
		RadiusEpsilon = CustomShrinkAmount;
		HeightEpsilon = CustomShrinkAmount;
		break;

	default:
		UE_LOG(LogCharacterMovement, Warning, TEXT("Unknown EShrinkCapsuleExtent in UCharacterMovementComponent::GetCapsuleExtent"));
		break;
	}

	// Don't shrink to zero extent.
	const float MinExtent = KINDA_SMALL_NUMBER * 10.f;
	CapsuleExtent.X = FMath::Max(CapsuleExtent.X - RadiusEpsilon, MinExtent);
	CapsuleExtent.Y = CapsuleExtent.X;
	CapsuleExtent.Z = FMath::Max(CapsuleExtent.Z - HeightEpsilon, MinExtent);

	return CapsuleExtent;
}


bool UCharacterMovementComponent::DoJump()
{
	if ( CharacterOwner )
	{
		// Don't jump if we can't move up/down.
		if (FMath::Abs(PlaneConstraintNormal.Z) != 1.f)
		{
			Velocity.Z = JumpZVelocity;
			SetMovementMode(MOVE_Falling);
			return true;
		}
	}
	
	return false;
}

FVector UCharacterMovementComponent::GetImpartedMovementBaseVelocity() const
{
	FVector Result = FVector::ZeroVector;
	if (CharacterOwner)
	{
		UPrimitiveComponent* MovementBase = CharacterOwner->GetMovementBase();
		if (MovementBase != NULL && (MovementBase->Mobility == EComponentMobility::Movable))
		{
			FVector BaseVelocity = MovementBase->GetComponentVelocity();
			if (BaseVelocity.IsZero() && MovementBase->GetOwner())
			{
				// Component might be moved manually (not by simulated physics or a movement component), see if the root component of the actor has a velocity.
				BaseVelocity = MovementBase->GetOwner()->GetVelocity();
			}

			if (bImpartBaseAngularVelocity)
			{
				const FVector BaseAngVel = MovementBase->GetPhysicsAngularVelocity();
				if (!BaseAngVel.IsZero())
				{
					const FVector CharacterBasePosition = (UpdatedComponent->GetComponentLocation() - FVector(0.f, 0.f, CharacterOwner->CapsuleComponent->GetScaledCapsuleHalfHeight()));
					const FVector RadialDistanceToBase = CharacterBasePosition - MovementBase->GetComponentLocation();
					const FVector BaseLinearVelFromAngularVel = FMath::DegreesToRadians(BaseAngVel) ^ RadialDistanceToBase;
					BaseVelocity += BaseLinearVelFromAngularVel;
				}
			}

			if (bImpartBaseVelocityX)
			{
				Result.X = BaseVelocity.X;
			}
			if (bImpartBaseVelocityY)
			{
				Result.Y = BaseVelocity.Y;
			}
			if (bImpartBaseVelocityZ)
			{
				Result.Z = BaseVelocity.Z;
			}
		}
	}
	
	return Result;
}

void UCharacterMovementComponent::Launch(FVector const& LaunchVel)
{
	PendingLaunchVelocity = LaunchVel;
}

void UCharacterMovementComponent::JumpOff(AActor* MovementBaseActor)
{
	if ( !bPerformingJumpOff )
	{
		bPerformingJumpOff = true;
		if ( CharacterOwner )
		{
			const float MaxSpeed = GetMaxSpeed() * 0.85f;
			Velocity += MaxSpeed * GetBestDirectionOffActor(MovementBaseActor);
			if ( Velocity.Size2D() > MaxSpeed )
			{
				Velocity = MaxSpeed * Velocity.SafeNormal();
			}
			Velocity.Z = JumpOffJumpZFactor * JumpZVelocity;
			SetMovementMode(MOVE_Falling);
		}
		bPerformingJumpOff = false;
	}
}

FVector UCharacterMovementComponent::GetBestDirectionOffActor(AActor* BaseActor) const
{
	// By default, just pick a random direction.  Derived character classes can choose to do more complex calculations,
	// such as finding the shortest distance to move in based on the BaseActor's Bounding Volume.
	const float RandAngle = FMath::DegreesToRadians(GetNetworkSafeRandomAngleDegrees());
	return FVector(FMath::Cos(RandAngle), FMath::Sin(RandAngle), 0.5f).SafeNormal();
}

float UCharacterMovementComponent::GetNetworkSafeRandomAngleDegrees() const
{
	float Angle = FMath::SRand() * 360.f;

	if (GetNetMode() > NM_Standalone)
	{
		// Networked game
		// Get a timestamp that is relatively close between client and server (within ping).
		FNetworkPredictionData_Server_Character const* ServerData = (HasPredictionData_Server() ? GetPredictionData_Server_Character() : NULL);
		FNetworkPredictionData_Client_Character const* ClientData = (HasPredictionData_Client() ? GetPredictionData_Client_Character() : NULL);

		float TimeStamp = Angle;
		if (ServerData)
		{
			TimeStamp = ServerData->CurrentClientTimeStamp;
		}
		else if (ClientData)
		{
			TimeStamp = ClientData->CurrentTimeStamp;
		}
		
		// Convert to degrees with a faster period.
		const float PeriodMult = 8.0f;
		Angle = TimeStamp * PeriodMult;
		Angle = FMath::Fmod(Angle, 360.f);
	}

	return Angle;
}


void UCharacterMovementComponent::SetDefaultMovementMode()
{
	// check for water volume
	if ( IsInWater() )
	{
		SetMovementMode(DefaultWaterMovementMode);
	}
	else if ( !CharacterOwner || !IsFalling() )
	{
		SetMovementMode(DefaultLandMovementMode);
	}
}

void UCharacterMovementComponent::SetMovementMode(EMovementMode NewMovementMode)
{
	if (!CharacterOwner)
	{
		return;
	}

	if (NewMovementMode == MOVE_Walking)
	{
		Velocity.Z = 0.f;
	}

	if (MovementMode == NewMovementMode)
	{
		return;
	}

	EMovementMode PrevMovementMode = MovementMode;
	MovementMode = NewMovementMode;
	if( MovementMode == MOVE_Walking )
	{	
		// Walking uses only XY velocity, and must be on a walkable floor, with a Base.
		Velocity.Z = 0.f;
		bCrouchMovesCharacterDown = true;
		if( UpdatedComponent )
		{
			FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, false);
			AdjustFloorHeight();
			
			SetBase(CurrentFloor.HitResult.Component.Get());

			// make sure we validate our new floor/base on initial entry of the walking physics
			bForceNextFloorCheck = true;
		}
	}
	else
	{
		CurrentFloor.Clear();
		bCrouchMovesCharacterDown = false;

		if (MovementMode == MOVE_Falling)
		{
			Velocity += GetImpartedMovementBaseVelocity();
			CharacterOwner->Falling();
		}
		SetBase(NULL);
	}

	// Notify the pawn that the movement mode has been changed
	CharacterOwner->OnMovementModeChanged(PrevMovementMode);

	// @todo UE4 do we need to disable ragdoll physics here? Should this function do nothing if in ragdoll?
}

void UCharacterMovementComponent::PerformAirControl(FVector Direction, float ZDiff)
{
	// use air control if low grav or above destination and falling towards it
	if ( CharacterOwner && Velocity.Z < 0.f && (ZDiff < 0.f || GetGravityZ() > 0.9f * GetWorld()->GetDefaultGravityZ()))
	{
		if ( ZDiff > 0.f )
		{
			if ( ZDiff > 2.f * GetMaxJumpHeight() )
			{
				if (PathFollowingComp.IsValid())
				{
					PathFollowingComp->AbortMove(TEXT("missed jump"));
				}
			}
		}
		else
		{
			if ( (Velocity.X == 0.f) && (Velocity.Y == 0.f) )
			{
				Acceleration = FVector::ZeroVector;
			}
			else
			{
				float Dist2D = Direction.Size2D();
				//Direction.Z = 0.f;
				Acceleration = Direction.SafeNormal() * GetModifiedMaxAcceleration();

				if ( (Dist2D < 0.5f * FMath::Abs(Direction.Z)) && ((Velocity | Direction) > 0.5f*FMath::Square(Dist2D)) )
				{
					Acceleration *= -1.f;
				}

				if ( Dist2D < 1.5f*CharacterOwner->CapsuleComponent->GetScaledCapsuleRadius() )
				{
					Velocity.X = 0.f;
					Velocity.Y = 0.f;
					Acceleration = FVector::ZeroVector;
				}
				else if ( (Velocity | Direction) < 0.f )
				{
					float M = FMath::Max(0.f, 0.2f - GetWorld()->DeltaTimeSeconds);
					Velocity.X *= M;
					Velocity.Y *= M;
				}
			}
		}
	}
}

void UCharacterMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsValid(CharacterOwner) || !UpdatedComponent)
	{
		return;
	}

	if (AvoidanceLockTimer > 0.0f)
	{
		AvoidanceLockTimer -= DeltaTime;
	}

	if (CharacterOwner->Role > ROLE_SimulatedProxy)
	{
		if (CharacterOwner->Role == ROLE_Authority)
		{
			// Check we are still in the world, and stop simulating if not.
			const bool bStillInWorld = (bCheatFlying || CharacterOwner->CheckStillInWorld());
			if (!bStillInWorld || !IsValid(CharacterOwner))
			{
				return;
			}
		}

		// If we are a client we might have received an update from the server.
		const bool bIsClient = (GetNetMode() == NM_Client && CharacterOwner->Role == ROLE_AutonomousProxy);
		if (bIsClient)
		{
			ClientUpdatePositionAfterServerUpdate();
		}

		// Allow root motion to move characters that have no controller.
		if( CharacterOwner->IsLocallyControlled() || bRunPhysicsWithNoController || (!CharacterOwner->Controller && CharacterOwner->IsPlayingRootMotion()) )
		{
			// We need to check the jump state before adjusting input acceleration, to minimize latency
			// and to make sure acceleration respects our potentially new falling state.
			CharacterOwner->CheckJumpInput(DeltaTime);

			// apply input to acceleration
			Acceleration = ScaleInputAcceleration(ConstrainInputAcceleration(GetInputVector()));
			ConsumeInputVector();

			if (CharacterOwner->Role == ROLE_Authority)
			{
				PerformMovement(DeltaTime);
			}
			else if (bIsClient)
			{
				ReplicateMoveToServer(DeltaTime, Acceleration);
			}

			CharacterOwner->ClearJumpInput();
		}
	}
	else if (CharacterOwner->Role == ROLE_SimulatedProxy)
	{
		AdjustProxyCapsuleSize();
		SimulatedTick(DeltaTime);
	}

	UpdateDefaultAvoidance();

	if (bEnablePhysicsInteraction)
	{
		// Apply downwards force when walking on top of physics objects
		if (CharacterOwner->GetMovementBase() != NULL)
		{
			UPrimitiveComponent* BaseComp = CharacterOwner->GetMovementBase();

			if (BaseComp->IsSimulatingPhysics())
			{
				UPrimitiveComponent* CharacterComp = CharacterOwner->Mesh.Get();

#if WITH_EDITOR
				// We need to calculate the mass here, as the bodies, when kinematic, and do not return proper mass
				float CharacterMass = CharacterComp != NULL ? CharacterComp->CalculateMass() : 1.0f;
#else
				float CharacterMass = 1.0f;
#endif

				float GravZ = GetGravityZ();
				BaseComp->AddForceAtLocation(FVector(0,0, GravZ * CharacterMass), CharacterComp->GetComponentLocation());
			}
		}

		ApplyRepulsionForce(DeltaTime);
	}
}


void UCharacterMovementComponent::AdjustProxyCapsuleSize()
{
	if (bShrinkProxyCapsule && CharacterOwner && CharacterOwner->Role == ROLE_SimulatedProxy)
	{
		bShrinkProxyCapsule = false;
		static const auto CVarShrinkRadius = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("p.NetProxyShrinkRadius"));
		static const auto CVarShrinkHalfHeight = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("p.NetProxyShrinkHalfHeight"));

		float ShrinkRadius = CVarShrinkRadius ? FMath::Max(0.f, CVarShrinkRadius->GetValueOnGameThread()) : 0.f;
		float ShrinkHalfHeight = CVarShrinkHalfHeight ? FMath::Max(0.f, CVarShrinkHalfHeight->GetValueOnGameThread()) : 0.f;

		if (ShrinkRadius == 0.f && ShrinkHalfHeight == 0.f)
		{
			return;
		}

		float Radius, HalfHeight;
		CharacterOwner->CapsuleComponent->GetUnscaledCapsuleSize(Radius, HalfHeight);
		const float ComponentScale = CharacterOwner->CapsuleComponent->GetShapeScale();

		if (ComponentScale <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		const float NewRadius = FMath::Max(0.f, Radius - ShrinkRadius / ComponentScale);
		const float NewHalfHeight = FMath::Max(0.f, HalfHeight - ShrinkHalfHeight / ComponentScale);

		if (NewRadius == 0.f || NewHalfHeight == 0.f)
		{
			UE_LOG(LogCharacterMovement, Warning, TEXT("Invalid attempt to shrink Proxy capsule for %s to zero dimension!"), *CharacterOwner->GetName());
			return;
		}

		UE_LOG(LogCharacterMovement, Verbose, TEXT("Shrinking capsule for %s from (r=%.3f, h=%.3f) to (r=%.3f, h=%.3f)"), *CharacterOwner->GetName(),
			Radius * ComponentScale, HalfHeight * ComponentScale, NewRadius * ComponentScale, NewHalfHeight * ComponentScale);

		CharacterOwner->CapsuleComponent->SetCapsuleSize(NewRadius, NewHalfHeight, true);
		CharacterOwner->CapsuleComponent->UpdateBounds();
	}	
}


void UCharacterMovementComponent::SimulatedTick(float DeltaSeconds)
{
	SCOPE_CYCLE_COUNTER(STAT_CharacterMovementSimulated);

	// If we are playing a RootMotion AnimMontage.
	if( CharacterOwner && CharacterOwner->IsPlayingRootMotion() )
	{
		UE_LOG(LogRootMotion, Verbose, TEXT("UCharacterMovementComponent::SimulatedTick"));

		// Tick animations before physics.
		if( CharacterOwner->Mesh )
		{
			CharacterOwner->Mesh->TickPose(DeltaSeconds);

			// Make sure animation didn't trigger an event that destroyed us
			if( !CharacterOwner || CharacterOwner->IsPendingKill() )
			{
				return;
			}
		}

		if( RootMotionParams.bHasRootMotion )
		{
			const FRotator OldRotation = CharacterOwner->GetActorRotation();
			const FVector OldLocation = CharacterOwner->GetActorLocation();
			SimulateRootMotion(DeltaSeconds, RootMotionParams.RootMotionTransform);
			// Root Motion has been used, clear
			RootMotionParams.Clear();

			// debug
			if( false )
			{
				const FRotator NewRotation = CharacterOwner->GetActorRotation();
				const FVector NewLocation = CharacterOwner->GetActorLocation();
				DrawDebugCoordinateSystem(GetWorld(), CharacterOwner->Mesh->GetComponentLocation() + FVector(0,0,1), NewRotation, 50.f, false);
				DrawDebugLine(GetWorld(), OldLocation, NewLocation, FColor::Red, true, 10.f);

				UE_LOG(LogRootMotion, Log,  TEXT("UCharacterMovementComponent::SimulatedTick DeltaMovement Translation: %s, Rotation: %s, MovementBase: %s"),
					*(NewLocation - OldLocation).ToCompactString(), *(NewRotation - OldRotation).GetNormalized().ToCompactString(), *GetNameSafe(CharacterOwner->GetMovementBase()) );
			}
		}

		// then, once our position is up to date with our animation, 
		// handle position correction if we have any pending updates received from the server.
		if( CharacterOwner && (CharacterOwner->RootMotionRepMoves.Num() > 0) )
		{
			CharacterOwner->SimulatedRootMotionPositionFixup(DeltaSeconds);
		}
	}
	else if( UpdatedComponent->IsSimulatingPhysics() || CharacterOwner->IsMatineeControlled()  )
	{
		PerformMovement(DeltaSeconds);
	}
	else
	{
		SimulateMovement(DeltaSeconds);
	}

	if( GetNetMode() == NM_Client )
	{
		SmoothClientPosition(DeltaSeconds);
	}
}

void UCharacterMovementComponent::SimulateRootMotion(float DeltaSeconds, const FTransform & LocalRootMotionTransform)
{
	if( CharacterOwner && CharacterOwner->Mesh && (DeltaSeconds > 0.f) )
	{
		// Convert Local Space Root Motion to world space. Do it right before used by physics to make sure we use up to date transforms, as translation is relative to rotation.
		const FTransform WorldSpaceRootMotionTransform = CharacterOwner->Mesh->ConvertLocalRootMotionToWorld(LocalRootMotionTransform);

		// Compute root motion velocity to be used by physics
		const FVector RootMotionVelocity = WorldSpaceRootMotionTransform.GetTranslation() / DeltaSeconds;
		// Do not override Velocity.Z if falling.
		Velocity = FVector(RootMotionVelocity.X, RootMotionVelocity.Y, (MovementMode == MOVE_Falling ? Velocity.Z : RootMotionVelocity.Z));

		StartNewPhysics(DeltaSeconds, 0);
		// fixme laurent - simulate movement seems to have step up issues? investigate as that would be cheaper to use.
		// 		SimulateMovement(DeltaSeconds);

		// Apply Root Motion rotation after movement is complete.
		const FRotator RootMotionRotation = WorldSpaceRootMotionTransform.GetRotation().Rotator();
		if( !RootMotionRotation.IsNearlyZero() )
		{
			const FRotator NewActorRotation = (CharacterOwner->GetActorRotation() + RootMotionRotation).GetNormalized();
			MoveUpdatedComponent(FVector::ZeroVector, NewActorRotation, true);
		}
	}
}


EMovementMode UCharacterMovementComponent::DetermineSimulatedMovementMode() const
{
	if (!CharacterOwner || !UpdatedComponent)
	{
		return MOVE_None;
	}
	
	// guess at movement mode
	if (CharacterOwner->GetMovementBase())
	{
		// if base, assume walking
		return MOVE_Walking;
	}
	else
	{
		// if no base, assume falling or swimming depending on physicsvolume (override to support flying, etc.)
		if (IsInWater())
		{
			return MOVE_Swimming;
		}
		else if (!CharacterOwner->bSimulateGravity)
		{
			return MOVE_Flying;
		}
		else
		{
			return MOVE_Falling;
		}
	}
}


void UCharacterMovementComponent::SimulateMovement(float DeltaSeconds)
{
	if (!CharacterOwner || !UpdatedComponent)
	{
		return;
	}

	// Workaround for replication not being updated initially
	if (CharacterOwner->ReplicatedMovement.Location.IsZero() &&
		CharacterOwner->ReplicatedMovement.Rotation.IsZero() &&
		CharacterOwner->ReplicatedMovement.LinearVelocity.IsZero())
	{
		return;
	}

	// If base is not resolved on the client, we should not try to simulate at all
	if (CharacterOwner->RelativeMovement.IsBaseUnresolved())
	{
		UE_LOG(LogCharacterMovement, Verbose, TEXT("Base for simulated character '%s' is not resolved on client, skipping SimulateMovement"), *CharacterOwner->GetName());
		return;
	}

	FScopedMovementUpdate ScopedMovementUpdate(UpdatedComponent, bEnableScopedMovementUpdates ? EScopedUpdate::DeferredUpdates : EScopedUpdate::ImmediateUpdates);

	// handle pending launches
	if ( !PendingLaunchVelocity.IsZero() && IsValid(UpdatedComponent->GetOwner()) )
	{
		// Handle pending Launch here
		Velocity = PendingLaunchVelocity;
		SetMovementMode(MOVE_Falling);
		PendingLaunchVelocity = FVector::ZeroVector;
	}

	// Movement mode is not replicated, guess at it.
	SetMovementMode(DetermineSimulatedMovementMode());
	if (MovementMode == MOVE_None)
	{
		return;
	}

	Acceleration = Velocity.SafeNormal();

	// simulated pawns predict location
	FStepDownResult StepDownResult;
	MoveSmooth(Velocity, DeltaSeconds, &StepDownResult);

	// if simulated gravity, find floor and check if falling
	if( CharacterOwner->bSimulateGravity && !CharacterOwner->bSimGravityDisabled && (MovementMode == MOVE_Walking || MovementMode == MOVE_Falling) )
	{
		const FVector CollisionCenter = UpdatedComponent->GetComponentLocation();
		if (StepDownResult.bComputedFloor)
		{
			CurrentFloor = StepDownResult.FloorResult;
		}
		else if (Velocity.Z <= 0.f)
		{
			FindFloor(CollisionCenter, CurrentFloor, Velocity.IsZero(), NULL);
		}
		else
		{
			CurrentFloor.Clear();
		}

		if (!CurrentFloor.IsWalkableFloor())
		{
			// No floor, must fall.
			Velocity = NewFallVelocity(Velocity, FVector(0.f,0.f,GetGravityZ()), DeltaSeconds);
			SetMovementMode(MOVE_Falling);
		}
		else
		{
			// Walkable floor
			if (MovementMode == MOVE_Walking)
			{
				AdjustFloorHeight();
				SetBase(CurrentFloor.HitResult.Component.Get());
			}
			else if (MovementMode == MOVE_Falling)
			{
				if (CurrentFloor.FloorDist <= MIN_FLOOR_DIST)
				{
					SetMovementMode(MOVE_Walking);
				}
				else
				{
					// Continue falling.
					Velocity = NewFallVelocity(Velocity, FVector(0.f,0.f,GetGravityZ()), DeltaSeconds);
					CurrentFloor.Clear();
				}
			}
		}
	}

	UpdateComponentVelocity();
}

void UCharacterMovementComponent::SetBase( UPrimitiveComponent* NewBase, bool bNotifyActor )
{
	CharacterOwner->SetBase(NewBase,bNotifyActor);
}


// @todo UE4 - handle lift moving up and down through encroachment
// @todo UE4 - also smoothly handle simulated pawns located relative to base
void UCharacterMovementComponent::UpdateBasedMovement(float DeltaSeconds)
{
	if ( !CharacterOwner || !UpdatedComponent )
	{
		return;
	}

	UPrimitiveComponent* MovementBase = CharacterOwner->GetMovementBase();
	if (!MovementBaseUtility::UseRelativePosition(MovementBase))
	{
		return;
	}

	if (!IsValid(MovementBase->GetOwner()))
	{
		SetBase(NULL);
		return;
	}

	// Ignore collision with bases during these movements.
	TGuardValue<EMoveComponentFlags> ScopedFlagRestore(MoveComponentFlags, MoveComponentFlags | MOVECOMP_IgnoreBases);

	FRotator ReducedRotation(0,0,0);
	const bool bRotationChanged = (OldBaseRotation != MovementBase->GetComponentRotation());
	if( bRotationChanged )
	{
		//@hack - Angles used to need to be reduced for sine and cosine table lookup, maybe remove this
		ReducedRotation = MovementBase->GetComponentRotation() - OldBaseRotation;
	}

	// only if base moved
	if ( bRotationChanged || (OldBaseLocation != MovementBase->GetComponentLocation()) )
	{
		// Calculate new transform matrix of base actor (ignoring scale).
		const FRotationTranslationMatrix OldLocalToWorld(OldBaseRotation, OldBaseLocation);
		const FRotationTranslationMatrix NewLocalToWorld(MovementBase->GetComponentRotation(), MovementBase->GetComponentLocation());

		FVector RotMotion(0.f);
		if( CharacterOwner->IsMatineeControlled() )
		{
			FRotationTranslationMatrix HardRelMatrix(CharacterOwner->RelativeMovement.Rotation, CharacterOwner->RelativeMovement.Location);
			const FMatrix NewWorldTM = HardRelMatrix * NewLocalToWorld;
			const FRotator NewWorldRot = bIgnoreBaseRotation ? CharacterOwner->GetActorRotation() : NewWorldTM.Rotator();
			MoveUpdatedComponent( NewWorldTM.GetOrigin() - CharacterOwner->GetActorLocation(), NewWorldRot, true );
		}
		else
		{
			FRotator finalRotation = CharacterOwner->GetActorRotation() + ReducedRotation;
			FRotator PawnOldRotation = CharacterOwner->GetActorRotation();
			CharacterOwner->FaceRotation(finalRotation, 0.f);
			finalRotation = CharacterOwner->GetActorRotation();

			if( bRotationChanged )
			{
				//@hack - Angles used to need to be reduced for sine and cosine table lookup, maybe remove this
				FRotator PawnReducedRotation = finalRotation - PawnOldRotation;
				UpdateBasedRotation(finalRotation, PawnReducedRotation);
			}
			FVector const LocalPos = OldLocalToWorld.InverseTransformPosition(CharacterOwner->GetActorLocation());
			FVector const NewWorldPos = ConstrainLocationToPlane(NewLocalToWorld.TransformPosition(LocalPos));
			FVector const Delta = ConstrainDirectionToPlane(NewWorldPos - CharacterOwner->GetActorLocation());

			// move attached actor
			if (bFastAttachedMove)
			{
				// we're trusting no other obstacle can prevent the move here
				UpdatedComponent->SetWorldLocationAndRotation(NewWorldPos, finalRotation, false);
			}
			else
			{
				MoveUpdatedComponent( Delta, finalRotation, true );
			}
		}
	}
}


void UCharacterMovementComponent::UpdateBasedRotation(FRotator &FinalRotation, const FRotator& ReducedRotation)
{
	AController* Controller = CharacterOwner ? CharacterOwner->Controller : NULL;
	float ControllerRoll = 0.f;
	if( Controller && !bIgnoreBaseRotation )
	{
		FRotator const ControllerRot = Controller->GetControlRotation();
		ControllerRoll = ControllerRot.Roll;
		Controller->SetControlRotation(ControllerRot + ReducedRotation);
	}

	// Remove roll
	FinalRotation.Roll = 0.f;
	if( Controller )
	{
		FinalRotation.Roll = CharacterOwner->GetActorRotation().Roll;
		FRotator NewRotation = Controller->GetControlRotation();
		NewRotation.Roll = ControllerRoll;
		Controller->SetControlRotation(NewRotation);
	}
}

void UCharacterMovementComponent::DisableMovement()
{
	if (CharacterOwner)
	{
		SetMovementMode(MOVE_None);
	}
	else
	{
		MovementMode = MOVE_None;
	}
}

void UCharacterMovementComponent::PerformMovement(float DeltaSeconds)
{
	SCOPE_CYCLE_COUNTER(STAT_CharacterMovementAuthority);

	if (!IsValid(CharacterOwner) || !UpdatedComponent)
	{
		return;
	}

	FScopedMovementUpdate ScopedMovementUpdate(UpdatedComponent, bEnableScopedMovementUpdates ? EScopedUpdate::DeferredUpdates : EScopedUpdate::ImmediateUpdates);

	// if ( CharacterOwner->Controller && Cast<APlayerController>(CharacterOwner->Controller) && ((CharacterOwner->Role != ROLE_Authority) || !CharacterOwner->IsLocallyControlled()) ) UE_LOG(LogNet, Log, TEXT("%s PerformMovement"), *CharacterOwner->GetName());
	UpdateBasedMovement(DeltaSeconds);

	// no movement if pawn owner left world, or currently doing physical simulation on UpdatedComponent
	if (MovementMode == MOVE_None || UpdatedComponent->IsSimulatingPhysics())
	{
		return;
	}

	FVector OldVelocity = Velocity;
	FVector OldLocation = CharacterOwner->GetActorLocation();

	bool bAllowedToCrouch = CanCrouchInCurrentState();
	if (!bAllowedToCrouch && IsCrouching())
	{
		UnCrouch(false);
	}
	else if( bWantsToCrouch && bAllowedToCrouch ) 
	{
		// players crouch by setting bWantsToCrouch to true
		if( !IsCrouching() )
		{
			Crouch(false);
		}
	}

	// handle pending launches
	if ( !PendingLaunchVelocity.IsZero() && UpdatedComponent && IsValid(UpdatedComponent->GetOwner()) )
	{
		// Handle pending Launch here
		Velocity = PendingLaunchVelocity;
		SetMovementMode(MOVE_Falling);
		PendingLaunchVelocity = FVector::ZeroVector;
	}

	// If using RootMotion, tick animations before running physics.
	if( !CharacterOwner->bClientUpdating && CharacterOwner->IsPlayingRootMotion() && CharacterOwner->Mesh )
	{
		CharacterOwner->Mesh->TickPose(DeltaSeconds);

		// Make sure animation didn't trigger an event that destroyed us
		if( !CharacterOwner || CharacterOwner->IsPendingKill() )
		{
			return;
		}

		// For local human clients, save off root motion data so it can be used by movement networking code.
		if( CharacterOwner->IsLocallyControlled() && (CharacterOwner->Role == ROLE_AutonomousProxy) )
		{
			CharacterOwner->ClientRootMotionParams = RootMotionParams;
		}
	}

	// if we're about to use root motion, convert it to world space first.
	if( HasRootMotion() )
	{
		USkeletalMeshComponent * SkelMeshComp = CharacterOwner->Mesh;
		if( SkelMeshComp )
		{
			// Convert Local Space Root Motion to world space. Do it right before used by physics to make sure we use up to date transforms, as translation is relative to rotation.
			RootMotionParams.Set( SkelMeshComp->ConvertLocalRootMotionToWorld(RootMotionParams.RootMotionTransform) );
			UE_LOG(LogRootMotion, Log,  TEXT("PerformMovement WorldSpaceRootMotion Translation: %s, Rotation: %s, Actor Facing: %s"),
				*RootMotionParams.RootMotionTransform.GetTranslation().ToCompactString(), *RootMotionParams.RootMotionTransform.GetRotation().Rotator().ToCompactString(), *CharacterOwner->GetActorRotation().Vector().ToCompactString());
		}

		// Then turn root motion to velocity to be used by various physics modes.
		if( DeltaSeconds > 0.f )
		{
			const FVector RootMotionVelocity = RootMotionParams.RootMotionTransform.GetTranslation() / DeltaSeconds;
			// Do not override Velocity.Z if in falling physics, we want to keep the effect of gravity.
			Velocity = FVector(RootMotionVelocity.X, RootMotionVelocity.Y, (MovementMode == MOVE_Falling ? Velocity.Z : RootMotionVelocity.Z));
		}
	}

	// NaN tracking
	if( Velocity.ContainsNaN() )
	{
		UE_LOG(LogCharacterMovement, Error, TEXT("UCharacterMovementComponent::PerformMovement Velocity contains NaN (%s). DeltaSeconds: %f"), 
			*Velocity.ToString(), DeltaSeconds );
		check( !Velocity.ContainsNaN() && "UCharacterMovementComponent::PerformMovement");
	}

	// change position
	StartNewPhysics(DeltaSeconds, 0);

	if (!IsValid(CharacterOwner))
	{
		return;
	}

	// uncrouch if no longer desiring crouch
	// or if not in walking or falling physics
	bAllowedToCrouch = CanCrouchInCurrentState();
	if (IsCrouching() && (!bAllowedToCrouch || !bWantsToCrouch))
	{
		UnCrouch(false);
	}

	if( CharacterOwner->Controller || bRunPhysicsWithNoController )
	{
		if (!HasRootMotion() && !CharacterOwner->IsMatineeControlled())
		{
			PhysicsRotation(DeltaSeconds);
		}
	}

	// Apply Root Motion rotation after movement is complete.
	if( HasRootMotion() )
	{
		const FRotator OldActorRotation = CharacterOwner->GetActorRotation();
		const FRotator RootMotionRotation = RootMotionParams.RootMotionTransform.GetRotation().Rotator();
		if( !RootMotionRotation.IsNearlyZero() )
		{
			const FRotator NewActorRotation = (OldActorRotation + RootMotionRotation).GetNormalized();
			MoveUpdatedComponent(FVector::ZeroVector, NewActorRotation, true);
		}

		// debug
		if( false )
		{
			const FVector ResultingLocation = CharacterOwner->GetActorLocation();
			const FRotator ResultingRotation = CharacterOwner->GetActorRotation();

			// Show current position
			DrawDebugCoordinateSystem(GetWorld(), CharacterOwner->Mesh->GetComponentLocation() + FVector(0,0,1), ResultingRotation, 50.f, false);

			// Show resulting delta move.
			DrawDebugLine(GetWorld(), OldLocation, ResultingLocation, FColor::Red, true, 10.f);

			// Log details.
			UE_LOG(LogRootMotion, Warning,  TEXT("PerformMovement Resulting DeltaMove Translation: %s, Rotation: %s, MovementBase: %s"),
				*(ResultingLocation - OldLocation).ToCompactString(), *(ResultingRotation - OldActorRotation).GetNormalized().ToCompactString(), *GetNameSafe(CharacterOwner->GetMovementBase()) );

			const FVector RMTranslation = RootMotionParams.RootMotionTransform.GetTranslation();
			const FRotator RMRotation = RootMotionParams.RootMotionTransform.GetRotation().Rotator();
			UE_LOG(LogRootMotion, Warning,  TEXT("PerformMovement Resulting DeltaError Translation: %s, Rotation: %s"),
				*(ResultingLocation - OldLocation - RMTranslation).ToCompactString(), *(ResultingRotation - OldActorRotation - RMRotation).GetNormalized().ToCompactString() );
		}

		// Root Motion has been used, clear
		RootMotionParams.Clear();
	}

	CharacterOwner->bSimulateGravity = (IsFalling() || IsMovingOnGround());

	// update relative location/rotation
	if (CharacterOwner->Role == ROLE_Authority || CharacterOwner->Role == ROLE_AutonomousProxy)
	{		
		UPrimitiveComponent* MovementBase = CharacterOwner->GetMovementBase();
		if (MovementBaseUtility::UseRelativePosition(MovementBase) && !CharacterOwner->IsMatineeControlled() && !Cast<USkeletalMeshComponent>(MovementBase))
		{
			OldBaseLocation = MovementBase->GetComponentLocation();
			CharacterOwner->RelativeMovement.Location = CharacterOwner->GetActorLocation() - OldBaseLocation;

			OldBaseRotation = MovementBase->GetComponentRotation();
			CharacterOwner->RelativeMovement.Rotation = (FRotationMatrix(CharacterOwner->GetActorRotation()) * FRotationMatrix(OldBaseRotation).GetTransposed()).Rotator();
		}
	}	

	// update component velocity
	UpdateComponentVelocity();
}


bool UCharacterMovementComponent::CanCrouchInCurrentState() const
{
	if (!CanEverCrouch())
	{
		return false;
	}

	return IsFalling() || IsMovingOnGround();
}


void UCharacterMovementComponent::Crouch(bool bClientSimulation)
{
	if (!CharacterOwner || !UpdatedComponent)
	{
		return;
	}

	// Do not perform if collision is already at desired size.
	if( CharacterOwner->CapsuleComponent->GetUnscaledCapsuleHalfHeight() == CrouchedHalfHeight )
	{
		return;
	}

	if (!CanCrouchInCurrentState())
	{
		return;
	}

	if (bClientSimulation && CharacterOwner->Role == ROLE_SimulatedProxy)
	{
		// restore collision size before crouching
		ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
		CharacterOwner->CapsuleComponent->SetCapsuleSize(DefaultCharacter->CapsuleComponent->GetUnscaledCapsuleRadius(), DefaultCharacter->CapsuleComponent->GetUnscaledCapsuleHalfHeight());
		bShrinkProxyCapsule = true;
	}

	// Change collision size to crouching dimensions
	const float ComponentScale = CharacterOwner->CapsuleComponent->GetShapeScale();
	const float OldUnscaledHalfHeight = CharacterOwner->CapsuleComponent->GetUnscaledCapsuleHalfHeight();
	CharacterOwner->CapsuleComponent->SetCapsuleSize(CharacterOwner->CapsuleComponent->GetUnscaledCapsuleRadius(), CrouchedHalfHeight);
	float HalfHeightAdjust = (OldUnscaledHalfHeight - CrouchedHalfHeight);
	float ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;

	if( !bClientSimulation )
	{
		// Crouching to a larger height? (this is rare)
		if (CrouchedHalfHeight > OldUnscaledHalfHeight)
		{
			static const FName NAME_CrouchTrace = FName(TEXT("CrouchTrace"));
			FCollisionQueryParams CapsuleParams(NAME_CrouchTrace, false, CharacterOwner);
			FCollisionResponseParams ResponseParam;
			InitCollisionParams(CapsuleParams, ResponseParam);
			const bool bEncroached = GetWorld()->OverlapTest(CharacterOwner->GetActorLocation() - FVector(0.f,0.f,ScaledHalfHeightAdjust), FQuat::Identity,
				UpdatedComponent->GetCollisionObjectType(), GetPawnCapsuleCollisionShape(SHRINK_None), CapsuleParams, ResponseParam);

			// If encroached, cancel
			if( bEncroached )
			{
				CharacterOwner->CapsuleComponent->SetCapsuleSize(CharacterOwner->CapsuleComponent->GetUnscaledCapsuleRadius(), OldUnscaledHalfHeight);
				return;
			}
		}

		if (bCrouchMovesCharacterDown)
		{
			// Intentionally not using MoveUpdatedComponent, where a horizontal plane constraint would prevent the base of the capsule from staying at the same spot.
			UpdatedComponent->MoveComponent(FVector(0.f, 0.f, -ScaledHalfHeightAdjust), CharacterOwner->GetActorRotation(), true);
		}

		CharacterOwner->bIsCrouched = true;
	}

	bForceNextFloorCheck = true;

	// OnStartCrouch takes the change from the Default size, not the current one (though they are usually the same).
	ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();
	HalfHeightAdjust = (DefaultCharacter->CapsuleComponent->GetUnscaledCapsuleHalfHeight() - CrouchedHalfHeight);
	ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;

	AdjustProxyCapsuleSize();
	CharacterOwner->OnStartCrouch( HalfHeightAdjust, ScaledHalfHeightAdjust );
}


void UCharacterMovementComponent::UnCrouch(bool bClientSimulation)
{
	if (!CharacterOwner || !UpdatedComponent)
	{
		return;
	}

	ACharacter* DefaultCharacter = CharacterOwner->GetClass()->GetDefaultObject<ACharacter>();

	// Do not perform if collision is already at desired size.
	if( CharacterOwner->CapsuleComponent->GetUnscaledCapsuleHalfHeight() == DefaultCharacter->CapsuleComponent->GetUnscaledCapsuleHalfHeight() )
	{
		return;
	}

	const float ComponentScale = CharacterOwner->CapsuleComponent->GetShapeScale();
	const float OldUnscaledHalfHeight = CharacterOwner->CapsuleComponent->GetUnscaledCapsuleHalfHeight();
	const float HalfHeightAdjust = DefaultCharacter->CapsuleComponent->GetUnscaledCapsuleHalfHeight() - OldUnscaledHalfHeight;
	const float ScaledHalfHeightAdjust = HalfHeightAdjust * ComponentScale;
	const FVector PawnLocation = CharacterOwner->GetActorLocation();

	// Grow to uncrouched size.
	check(CharacterOwner->CapsuleComponent);
	bool bUpdateOverlaps = false;
	CharacterOwner->CapsuleComponent->SetCapsuleSize(DefaultCharacter->CapsuleComponent->GetUnscaledCapsuleRadius(), DefaultCharacter->CapsuleComponent->GetUnscaledCapsuleHalfHeight(), bUpdateOverlaps);
	CharacterOwner->CapsuleComponent->UpdateBounds(); // Force an update of the bounds with the new dimensions

	if( !bClientSimulation )
	{
		UPrimitiveComponent* OldBaseComponent = CharacterOwner->GetMovementBase();
		FFindFloorResult OldFloor = CurrentFloor;
		SetBase(NULL,false);

		// Try to stay in place and see if the larger capsule fits. We use a slightly taller capsule to avoid penetration.
		static const FName NAME_CrouchTrace = FName(TEXT("CrouchTrace"));
		const float SweepInflation = KINDA_SMALL_NUMBER;
		FCollisionQueryParams CapsuleParams(NAME_CrouchTrace, false, CharacterOwner);
		FCollisionResponseParams ResponseParam;
		InitCollisionParams(CapsuleParams, ResponseParam);
		const FCollisionShape StandingCapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_HeightCustom, -SweepInflation);
		const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();
		// Perf: Avoid this test when on the ground, because it almost always fails and we should just continue to the next step.
		bool bEncroached = true;
		if (!IsMovingOnGround())
		{
			bEncroached = GetWorld()->OverlapTest(PawnLocation, FQuat::Identity, CollisionChannel, StandingCapsuleShape, CapsuleParams, ResponseParam);
		}

		if (bEncroached)
		{
			// Try adjusting capsule position to see if we can avoid encroachment.
			if (ScaledHalfHeightAdjust > 0.f)
			{
				// Shrink to a short capsule, sweep down to base to find where that would hit something, and then try to stand up from there.
				float PawnRadius, PawnHalfHeight;
				CharacterOwner->CapsuleComponent->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);
				const float ShrinkHalfHeight = PawnHalfHeight - PawnRadius;
				const float TraceDist = PawnHalfHeight - ShrinkHalfHeight;
				const FVector Down = FVector(0.f, 0.f, -TraceDist);

				FHitResult Hit(1.f);
				const FCollisionShape ShortCapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_HeightCustom, ShrinkHalfHeight);
				const bool bBlockingHit = GetWorld()->SweepSingle(Hit, PawnLocation, PawnLocation + Down, FQuat::Identity, CollisionChannel, ShortCapsuleShape, CapsuleParams);
				if (Hit.bStartPenetrating)
				{
					bEncroached = true;
				}
				else
				{
					// Compute where the base of the sweep ended up, and see if we can stand there
					const float DistanceToBase = (Hit.Time * TraceDist) + ShortCapsuleShape.Capsule.HalfHeight;
					const FVector NewLoc = FVector(PawnLocation.X, PawnLocation.Y, PawnLocation.Z - DistanceToBase + PawnHalfHeight + SweepInflation + MIN_FLOOR_DIST / 2.f);
					bEncroached = GetWorld()->OverlapTest(NewLoc, FQuat::Identity, CollisionChannel, StandingCapsuleShape, CapsuleParams);
					if (!bEncroached)
					{
						// Intentionally not using MoveUpdatedComponent, where a horizontal plane constraint would prevent the base of the capsule from staying at the same spot.
						UpdatedComponent->MoveComponent(NewLoc - PawnLocation, CharacterOwner->GetActorRotation(), false);
						const bool bBaseChanged = (Hit.Component.Get() != OldBaseComponent);
						CurrentFloor.SetFromSweep(Hit, 0.f, IsWalkable(Hit));
						SetBase(Hit.Component.Get(), bBaseChanged);
					}
				}
			}

			// If still encroached then abort.
			if (bEncroached)
			{
				CharacterOwner->CapsuleComponent->SetCapsuleSize(CharacterOwner->CapsuleComponent->GetUnscaledCapsuleRadius(), OldUnscaledHalfHeight, false);
				CharacterOwner->CapsuleComponent->UpdateBounds(); // Update bounds again back to old value
				CurrentFloor = OldFloor;
				SetBase(OldBaseComponent,false);
				return;
			}
		}

		CharacterOwner->bIsCrouched = false;
	}	
	else
	{
		bShrinkProxyCapsule = true;
	}

	// now call SetCapsuleSize() to cause touch/untouch events
	bUpdateOverlaps = true;
	CharacterOwner->CapsuleComponent->SetCapsuleSize(DefaultCharacter->CapsuleComponent->GetUnscaledCapsuleRadius(), DefaultCharacter->CapsuleComponent->GetUnscaledCapsuleHalfHeight(), bUpdateOverlaps);

	bForceNextFloorCheck = true;
	AdjustProxyCapsuleSize();
	CharacterOwner->OnEndCrouch( HalfHeightAdjust, ScaledHalfHeightAdjust );
}

void UCharacterMovementComponent::StartNewPhysics(float deltaTime, int32 Iterations)
{
	if ( (deltaTime < 0.0003f) || (Iterations > 7) || !CharacterOwner)
		return;

	if(UpdatedComponent != NULL && UpdatedComponent->IsSimulatingPhysics())
	{
		UE_LOG(LogCharacterMovement, Log, TEXT("UCharacterMovementComponent::StartNewPhysics: UpdateComponent (%s) is simulating physics - aborting."), *UpdatedComponent->GetPathName());
		return;
	}

	bMovementInProgress = true;

	switch ( MovementMode )
	{
	case MOVE_None:
		return;
	case MOVE_Walking:
		PhysWalking(deltaTime, Iterations);
		break;
	case MOVE_Falling:
		PhysFalling(deltaTime, Iterations);
		break;
	case MOVE_Flying:
		PhysFlying(deltaTime, Iterations);
		break;
	case MOVE_Swimming:
		PhysSwimming(deltaTime, Iterations);
		break;
	case MOVE_Custom:
		PhysCustom(deltaTime, Iterations);
		break;
	default:
		UE_LOG(LogCharacterMovement, Warning, TEXT("%s has unsupported movement mode %d"), *CharacterOwner->GetName(), int32(MovementMode));
		SetMovementMode(MOVE_None);
		break;
	}

	bMovementInProgress = false;
	if ( bDeferUpdateMoveComponent )
	{
		SetUpdatedComponent(DeferredUpdatedMoveComponent);
	}
}

float UCharacterMovementComponent::GetGravityZ() const
{
	return Super::GetGravityZ() * GravityScale;
}

float UCharacterMovementComponent::GetMaxSpeed() const
{
	float Result = MaxWalkSpeed;
	if ( MovementMode == MOVE_Flying )
	{
		Result = MaxFlySpeed;
	}
	else if ( MovementMode == MOVE_Swimming )
	{
		Result = MaxSwimSpeed;
	}
	return Result;
}


bool UCharacterMovementComponent::ResolvePenetration(const FVector& Adjustment, const FHitResult& Hit, const FRotator& NewRotation)
{
	// If movement occurs, mark that we teleported, so we don't incorrectly adjust velocity based on a potentially very different movement than our movement direction.
	bJustTeleported |= Super::ResolvePenetration(Adjustment, Hit, NewRotation);
	return bJustTeleported;
}


FVector UCharacterMovementComponent::GetPenetrationAdjustment(const FHitResult& Hit) const
{
	FVector Adjustment = Super::GetPenetrationAdjustment(Hit);

	if (Adjustment.Z > 0.f && IsMovingOnGround())
	{
		// Don't allow upward adjustments to move us high off the ground.
		// Floor height adjustments will take care of penetrations on the lower portion of the capsule.
		Adjustment.Z = 0.f;
	}

	return Adjustment;
}


float UCharacterMovementComponent::SlideAlongSurface(const FVector& Delta, float Time, const FVector& InNormal, FHitResult &Hit, bool bHandleImpact)
{
	if (!Hit.bBlockingHit)
	{
		return 0.f;
	}

	FVector Normal(InNormal);
	if (IsMovingOnGround())
	{
		// We don't want to be pushed up an unwalkable surface.
		if (Normal.Z > 0.f)
		{
			if (!IsWalkable(Hit))
			{
				Normal = Normal.SafeNormal2D();
			}
		}
		else if (Normal.Z < -KINDA_SMALL_NUMBER)
		{
			// Don't push down into the floor when the impact is on the upper portion of the capsule.
			if (CurrentFloor.FloorDist < MIN_FLOOR_DIST && CurrentFloor.bBlockingHit)
			{
				const FVector FloorNormal = CurrentFloor.HitResult.Normal;
				const bool bFloorOpposedToMovement = (Delta | FloorNormal) < 0.f && (FloorNormal.Z < 1.f - DELTA);
				if (bFloorOpposedToMovement)
				{
					Normal = FloorNormal;
				}
				
				Normal = Normal.SafeNormal2D();
			}
		}
	}

	return Super::SlideAlongSurface(Delta, Time, Normal, Hit, bHandleImpact);
}


void UCharacterMovementComponent::TwoWallAdjust(FVector &Delta, const FHitResult& Hit, const FVector &OldHitNormal) const
{
	FVector InDelta = Delta;
	Super::TwoWallAdjust(Delta, Hit, OldHitNormal);

	if (IsMovingOnGround())
	{
		// Allow slides up walkable surfaces, but not unwalkable ones (treat those as vertical barriers).
		if (Delta.Z > 0.f)
		{
			if ((Hit.Normal.Z >= WalkableFloorZ || IsWalkable(Hit)) && Hit.Normal.Z > KINDA_SMALL_NUMBER)
			{
				// Maintain horizontal velocity
				const float Time = (1.f - Hit.Time);
				const FVector ScaledDelta = Delta.SafeNormal() * InDelta.Size();
				Delta = FVector(InDelta.X, InDelta.Y, ScaledDelta.Z / Hit.Normal.Z) * Time;
			}
			else
			{
				Delta.Z = 0.f;
			}
		}
		else if (Delta.Z < 0.f)
		{
			// Don't push down into the floor.
			if (CurrentFloor.FloorDist < MIN_FLOOR_DIST && CurrentFloor.bBlockingHit)
			{
				Delta.Z = 0.f;
			}
		}
	}
}


FVector UCharacterMovementComponent::ComputeSlideVector(const FVector& InDelta, const float Time, const FVector& Normal, const FHitResult& Hit) const
{
	const bool bFalling = IsFalling();
	FVector Delta = InDelta;

	// Don't make impacts on the upper hemisphere feel so much like a capsule
	if (bFalling && Delta.Z > 0.f)
	{
		if (Hit.Normal.Z < KINDA_SMALL_NUMBER)
		{
			float PawnRadius, PawnHalfHeight;
			CharacterOwner->CapsuleComponent->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);
			const float UpperHemisphereZ = UpdatedComponent->GetComponentLocation().Z + PawnHalfHeight - PawnRadius;
			if (Hit.ImpactPoint.Z > UpperHemisphereZ + KINDA_SMALL_NUMBER && IsWithinEdgeTolerance(Hit.Location, Hit.ImpactPoint, PawnRadius))
			{
				Delta = AdjustUpperHemisphereImpact(Delta, Hit);
			}
		}
	}

	FVector Result = Super::ComputeSlideVector(Delta, Time, Normal, Hit);

	// prevent boosting up slopes
	if ( bFalling && Result.Z > 0.f )
	{
		if (Delta.Z < 0.f && (Hit.ImpactNormal.Z < MAX_STEP_SIDE_Z))
		{
			// We were moving downward, but a slide was going to send us upward. We want to aim
			// straight down for the next move to make sure we get the most upward-facing opposing normal.
			Result = FVector(0.f, 0.f, Delta.Z);
		}
		else
		{
			Result.Z = FMath::Min(Result.Z, Delta.Z * Time);
		}
	}

	return Result;
}

FVector UCharacterMovementComponent::AdjustUpperHemisphereImpact(const FVector& Delta, const FHitResult& Hit) const
{
	const float ZScale = FMath::Clamp(1.f - (FMath::Abs(Hit.Normal.Z) * UpperImpactNormalScale), 0.f, 1.f);
	return FVector(Delta.X, Delta.Y, Delta.Z * ZScale);
}

FVector UCharacterMovementComponent::NewFallVelocity(FVector OldVelocity, FVector OldAcceleration, float timeTick)
{
	return OldVelocity + (OldAcceleration * timeTick);
}


float UCharacterMovementComponent::ImmersionDepth()
{
	float depth = 0.f;

	if ( CharacterOwner && GetPhysicsVolume()->bWaterVolume )
	{
		const float CollisionHeight = CharacterOwner->GetSimpleCollisionRadius();
		const float CollisionRadius = CharacterOwner->GetSimpleCollisionHalfHeight();

		if ( (CollisionHeight == 0.f) || (Buoyancy == 0.f) )
		{
			depth = 1.f;
		}
		else
		{
			UBrushComponent* VolumeBrushComp = GetPhysicsVolume()->BrushComponent;
			FHitResult Hit(1.f);
			if ( VolumeBrushComp )
			{
				const FVector TraceStart = CharacterOwner->GetActorLocation() + FVector(0.f,0.f,CollisionHeight);
				const FVector TraceEnd = CharacterOwner->GetActorLocation() - FVector(0.f,0.f,CollisionHeight);

				const static FName MovementComp_Character_ImmersionDepthName(TEXT("MovementComp_Character_ImmersionDepth"));
				FCollisionQueryParams NewTraceParams( MovementComp_Character_ImmersionDepthName, true );

				VolumeBrushComp->LineTraceComponent( Hit, TraceStart, TraceEnd, NewTraceParams );
			}

			depth = (Hit.Time == 1.f) ? 1.f : (1.f - Hit.Time);
		}
	}
	return depth;
}

bool UCharacterMovementComponent::IsFlying() const
{
	if (!CharacterOwner || !UpdatedComponent)
	{
		return false;
	}

	return MovementMode == MOVE_Flying;
}



bool UCharacterMovementComponent::IsMovingOnGround() const
{
	if (!CharacterOwner || !UpdatedComponent)
	{
		return false;
	}

	return MovementMode == MOVE_Walking;
}



bool UCharacterMovementComponent::IsFalling() const
{
	if (!CharacterOwner || !UpdatedComponent)
	{
		return false;
	}

	return MovementMode == MOVE_Falling;
}

bool UCharacterMovementComponent::IsSwimming() const
{
	if (!CharacterOwner || !UpdatedComponent)
	{
		return false;
	}

	return MovementMode == MOVE_Swimming;
}

bool UCharacterMovementComponent::IsCrouching() const
{
	return CharacterOwner && CharacterOwner->bIsCrouched;
}

void UCharacterMovementComponent::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
	// Do not update velocity when using root motion
	if (!CharacterOwner || HasRootMotion())
	{
		return;
	}

	Friction = FMath::Max(0.f, Friction);
	const float MaxAccel = GetModifiedMaxAcceleration();
	const float MaxSpeed = GetModifiedMaxSpeed();
	const float MaxSpeedSquared = FMath::Square(MaxSpeed);

	// Check if path following requested movement
	if ( !ApplyRequestedMove(Velocity, Acceleration, DeltaTime, MaxAccel, MaxSpeed) )
	{
		// Update Velocity
		if (bForceMaxAccel)
		{
			// Force acceleration at full speed.
			// In consideration order for direction: Acceleration, then Velocity, then Pawn's rotation.
			if (Acceleration.SizeSquared() > SMALL_NUMBER)
			{
				Acceleration = SafeNormalPrecise(Acceleration) * MaxAccel;
			}
			else 
			{
				Acceleration = MaxAccel * (Velocity.SizeSquared() < SMALL_NUMBER ? CharacterOwner->GetActorRotation().Vector() : SafeNormalPrecise(Velocity));
			}
		}

		// Apply braking or deceleration
		const bool bVelocityOverMax = IsExceedingMaxSpeed(MaxSpeed);
		if (Acceleration.IsZero() || bVelocityOverMax)
		{
			const FVector OldVelocity = Velocity;
			ApplyVelocityBraking(DeltaTime, Friction, BrakingDeceleration);
	
			// Don't allow braking to lower us below max speed if we started above it.
			if (bVelocityOverMax && Velocity.SizeSquared() < MaxSpeedSquared)
			{
				Velocity = SafeNormalPrecise(OldVelocity) * MaxSpeed;
			}
		}
		else
		{
			const FVector AccelDir = SafeNormalPrecise(Acceleration);
			const float VelSize = Velocity.Size();
			Velocity = Velocity - (Velocity - AccelDir * VelSize) * DeltaTime * Friction;
		}

		// Apply fluid friction
		if (bFluid)
		{
			Velocity = Velocity * (1.f - Friction * DeltaTime);
		}

		// Apply input acceleration
		const float NewMaxSpeed = (IsExceedingMaxSpeed(MaxSpeed)) ? Velocity.Size() : MaxSpeed;
		Velocity += Acceleration * DeltaTime;
		Velocity = ClampMaxSizePrecise(Velocity, NewMaxSpeed);
	}

	if (bUseRVOAvoidance && CharacterOwner->bUseAvoidancePathing)
	{
		CalcAvoidanceVelocity(DeltaTime);
	}
}

bool UCharacterMovementComponent::ApplyRequestedMove(FVector& NewVelocity, FVector& NewAcceleration, float DeltaTime, float MaxAccel, float MaxSpeed)
{
	if (bHasRequestedVelocity)
	{
		bHasRequestedVelocity = false;

		float RequestedSpeed = RequestedVelocity.Size();
		if (RequestedSpeed < KINDA_SMALL_NUMBER)
		{
			return true;
		}

		const FVector RequestedMoveDir = RequestedVelocity / RequestedSpeed;

		RequestedSpeed = bRequestedMoveWithMaxSpeed ? MaxSpeed : FMath::Min(MaxSpeed, RequestedSpeed);
		const FVector MoveVelocity = RequestedMoveDir * RequestedSpeed;

		NewAcceleration = MoveVelocity / DeltaTime;
		if (NewAcceleration.SizeSquared() > FMath::Square(MaxAccel) || bForceMaxAccel)
		{
			NewAcceleration = RequestedMoveDir * MaxAccel;
		}

		// use MaxAcceleration to limit speed increase, 2% buffer
		const float CurrentSpeedSq = Velocity.SizeSquared();
		if (bRequestedMoveUseAcceleration && CurrentSpeedSq < FMath::Square(RequestedSpeed * 0.98f))
		{
			const float ActualSpeed = FMath::Min(RequestedSpeed, FMath::Sqrt(CurrentSpeedSq) + DeltaTime * MaxAccel);
			NewVelocity = RequestedMoveDir * ActualSpeed;
		}
		else
		{
			// don't affect deceleration, agent needs to reach end of path without sliding though
			NewVelocity = MoveVelocity;
		}

		return true;
	}

	return false;
}

void UCharacterMovementComponent::RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed)
{
	if (MoveVelocity.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		return;
	}

	if (IsFalling())
	{
		const FVector FallVelocity = MoveVelocity.ClampMaxSize(GetModifiedMaxSpeed());
		PerformAirControl(FallVelocity, FallVelocity.Z);
		return;
	}

	RequestedVelocity = MoveVelocity;
	bHasRequestedVelocity = true;
	bRequestedMoveWithMaxSpeed = bForceMaxSpeed;

	if (IsMovingOnGround())
	{
		RequestedVelocity.Z = 0.0f;
	}
}

bool UCharacterMovementComponent::CanStopPathFollowing() const
{
	return !IsFalling();
}

void UCharacterMovementComponent::CalcAvoidanceVelocity(float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_ObstacleAvoidance);

	UAvoidanceManager* AvoidanceManager = GetWorld()->GetAvoidanceManager();
	if (AvoidanceWeight >= 1.0f || AvoidanceManager == NULL || GetCharacterOwner() == NULL)
	{
		return;
	}

	if (GetCharacterOwner()->Role != ROLE_Authority)
	{
		return;
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	const bool bShowDebug = AvoidanceManager->IsDebugEnabled(AvoidanceUID);
#endif

	//Adjust velocity only if we're in "Walking" mode. We should also check if we're dazed, being knocked around, maybe off-navmesh, etc.
	UCapsuleComponent *OurCapsule = GetCharacterOwner()->CapsuleComponent.Get();
	if (!Velocity.IsZero() && MovementMode == MOVE_Walking && OurCapsule)
	{
		//See if we're doing a locked avoidance move already, and if so, skip the testing and just do the move.
		if (AvoidanceLockTimer > 0.0f)
		{
			Velocity = AvoidanceLockVelocity;
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			if (bShowDebug)
			{
				DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + Velocity, FColor(0,0,255), true, 0.5f, SDPG_MAX);
			}
#endif
		}
		else
		{
			FNavAvoidanceData currentData;
			currentData.Init(AvoidanceManager, GetActorLocation(),
				OurCapsule->GetScaledCapsuleRadius(), OurCapsule->GetScaledCapsuleHalfHeight(),
				Velocity, AvoidanceWeight, AvoidanceGroup.Packed, GroupsToAvoid.Packed, GroupsToIgnore.Packed);

			FVector NewVelocity = AvoidanceManager->GetAvoidanceVelocityIgnoringUID(currentData, AvoidanceManager->DeltaTimeToPredict, AvoidanceUID);
			if (bUseRVOPostProcess)
			{
				PostProcessAvoidanceVelocity(NewVelocity);
			}

			if (!NewVelocity.Equals(Velocity))		//Really want to branch hint that this will probably not pass
			{
				//Had to divert course, lock this avoidance move in for a short time. This will make us a VO, so unlocked others will know to avoid us.
				Velocity = NewVelocity;
				SetAvoidanceVelocityLock(AvoidanceManager, AvoidanceManager->LockTimeAfterAvoid);
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
				if (bShowDebug)
				{
					DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + Velocity, FColor(255,0,0), true, 0.05f, SDPG_MAX, 10.0f);
				}
#endif
			}
			else
			{
				//Although we didn't divert course, our velocity for this frame is decided. We will not reciprocate anything further, so treat as a VO for the remainder of this frame.
				SetAvoidanceVelocityLock(AvoidanceManager, AvoidanceManager->LockTimeAfterClean);	//10 ms of lock time should be adequate.
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
				if (bShowDebug)
				{
					//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + Velocity, FColor(0,255,0), true, 0.05f, SDPG_MAX, 10.0f);
				}
#endif
			}
		}
		//RickH - We might do better to do this later in our update
		AvoidanceManager->UpdateRVO(AvoidanceUID, GetActorLocation(),
			OurCapsule->GetScaledCapsuleRadius(), OurCapsule->GetScaledCapsuleHalfHeight() * 2.0f,
			Velocity, AvoidanceWeight, AvoidanceGroup.Packed, GroupsToAvoid.Packed, GroupsToIgnore.Packed);

		bWasAvoidanceUpdated = true;
	}
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	else if (bShowDebug)
	{
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + Velocity, FColor(255,255,0), true, 0.05f, SDPG_MAX);
	}

	if (bShowDebug)
	{
		FVector UpLine(0,0,500);
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + UpLine, (AvoidanceLockTimer > 0.01f) ? FColor(255,0,0) : FColor(0,0,255), true, 0.05f, SDPG_MAX, 5.0f);
	}
#endif
}

void UCharacterMovementComponent::PostProcessAvoidanceVelocity(FVector& NewVelocity)
{
	// empty in base class
}

void UCharacterMovementComponent::UpdateDefaultAvoidance()
{
	if (!bUseRVOAvoidance)
	{
		return;
	}

	SCOPE_CYCLE_COUNTER(STAT_AI_ObstacleAvoidance);

	UAvoidanceManager* AvoidanceManager = GetWorld()->GetAvoidanceManager();
	if (AvoidanceManager && !bWasAvoidanceUpdated)
	{
		if (UCapsuleComponent *OurCapsule = GetCharacterOwner()->CapsuleComponent.Get())
		{
			AvoidanceManager->UpdateRVO(AvoidanceUID, GetActorLocation(),
				OurCapsule->GetScaledCapsuleRadius(), OurCapsule->GetScaledCapsuleHalfHeight() * 2.0f,
				Velocity, AvoidanceWeight, AvoidanceGroup.Packed, GroupsToAvoid.Packed, GroupsToIgnore.Packed);

			//Consider this a clean move because we didn't even try to avoid.
			SetAvoidanceVelocityLock(AvoidanceManager, AvoidanceManager->LockTimeAfterClean);
		}
	}

	bWasAvoidanceUpdated = false;		//Reset for next frame
}

void UCharacterMovementComponent::SetAvoidanceVelocityLock(class UAvoidanceManager* Avoidance, float Duration)
{
	Avoidance->OverrideToMaxWeight(AvoidanceUID, Duration);
	AvoidanceLockVelocity = Velocity;
	AvoidanceLockTimer = Duration;
}

void UCharacterMovementComponent::NotifyBumpedPawn(APawn* BumpedPawn)
{
	Super::NotifyBumpedPawn(BumpedPawn);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	UAvoidanceManager* Avoidance = GetWorld()->GetAvoidanceManager();
	const bool bShowDebug = Avoidance && Avoidance->IsDebugEnabled(AvoidanceUID);
	if (bShowDebug)
	{
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + FVector(0,0,500), (AvoidanceLockTimer > 0) ? FColor(255,64,64) : FColor(64,64,255), true, 2.0f, SDPG_MAX, 20.0f);
	}
#endif

	// Unlock avoidance move. This mostly happens when two pawns who are locked into avoidance moves collide with each other.
	AvoidanceLockTimer = 0.0f;
}

float UCharacterMovementComponent::GetMaxJumpHeight() const
{
	const float Gravity = GetGravityZ();
	if (FMath::Abs(Gravity) > KINDA_SMALL_NUMBER)
	{
		return FMath::Square(JumpZVelocity) / (-2.f * Gravity);
	}
	else
	{
		return 0.f;
	}
}

float UCharacterMovementComponent::GetModifiedMaxAcceleration() const
{
	return CharacterOwner ? MaxAcceleration * GetMaxSpeedModifier() : 0.f;
}

FVector UCharacterMovementComponent::GetCurrentAcceleration() const
{
	return Acceleration;
}

void UCharacterMovementComponent::ApplyVelocityBraking(float DeltaTime, float Friction, float BrakingDeceleration)
{
	if( Velocity.IsZero() || (!CharacterOwner && !HasRootMotion()) )
	{
		return;
	}

	const	FVector OldVel = Velocity;
	FVector SumVel = FVector::ZeroVector;

	// subdivide braking to get reasonably consistent results at lower frame rates
	// (important for packet loss situations w/ networking)
	float RemainingTime = DeltaTime;
	const float TimeStep = 0.03f;

	// Decelerate to brake to a stop
	const FVector RevAccel = (-1.f * BrakingDeceleration) * SafeNormalPrecise(Velocity);
	while( RemainingTime > 0.f )
	{
		float dt = ((RemainingTime > TimeStep) ? TimeStep : RemainingTime);
		RemainingTime -= dt;

		// apply friction and braking
		Velocity = Velocity + ((-2.f * Friction) * Velocity + RevAccel) * dt ; 
		if( (Velocity | OldVel) > 0.f )
		{
			SumVel += dt * Velocity/DeltaTime;
		}
		else
		{
			break;
		}
	}
	Velocity = SumVel;

	// brake to a stop, not backwards
	if( ((OldVel | Velocity) < 0.f)	|| (Velocity.SizeSquared() <= FMath::Square(BRAKE_TO_STOP_VELOCITY)) )
	{
		Velocity = FVector::ZeroVector;
	}
}

void UCharacterMovementComponent::PhysFlying(float deltaTime, int32 Iterations)
{
	if( !HasRootMotion() )
	{
		if( bCheatFlying && Acceleration.IsZero() )
		{
			Velocity = FVector::ZeroVector;
		}
		const float Friction = 0.5f * GetPhysicsVolume()->FluidFriction;
		CalcVelocity(deltaTime, Friction, true, BrakingDecelerationFlying);
	}

	Iterations++;
	bJustTeleported = false;

	FVector OldLocation = CharacterOwner->GetActorLocation();
	const FVector Adjusted = Velocity * deltaTime;
	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(Adjusted, CharacterOwner->GetActorRotation(), true, Hit);

	if( Hit.Time < 1.f && CharacterOwner )
	{
		FVector GravDir = FVector(0.f,0.f,-1.f);
		FVector VelDir = Velocity.SafeNormal();
		float UpDown = GravDir | VelDir;

		bool bSteppedUp = false;
		if( (FMath::Abs(Hit.ImpactNormal.Z) < 0.2f) && (UpDown < 0.5f) && (UpDown > -0.2f) && CanStepUp(Hit))
		{
			float stepZ = CharacterOwner->GetActorLocation().Z;
			bSteppedUp = StepUp(GravDir, Adjusted * (1.f - Hit.Time), Hit);
			if (bSteppedUp)
			{
				OldLocation.Z = CharacterOwner->GetActorLocation().Z + (OldLocation.Z - stepZ);
			}
		}
		
		if (!bSteppedUp)
		{
			//adjust and try again
			HandleImpact(Hit, deltaTime, Adjusted);
			SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
		}
	}
	
	if( !bJustTeleported && !HasRootMotion() && CharacterOwner )
	{
		Velocity = (CharacterOwner->GetActorLocation() - OldLocation) / deltaTime;
	}
}


void UCharacterMovementComponent::PhysSwimming(float deltaTime, int32 Iterations)
{
	float NetFluidFriction  = 0.f;
	float Depth = ImmersionDepth();
	float NetBuoyancy = Buoyancy * Depth;
	if( !HasRootMotion() && (Velocity.Z > 0.5f*GetMaxSpeed()) && (NetBuoyancy != 0.f) )
	{
		//damp positive Z out of water
		Velocity.Z = Velocity.Z * Depth;
	}
	Iterations++;
	FVector OldLocation = CharacterOwner->GetActorLocation();
	bJustTeleported = false;
	if( !HasRootMotion() )
	{
		const float Friction = 0.5f * GetPhysicsVolume()->FluidFriction * Depth;
		CalcVelocity(deltaTime, Friction, true, BrakingDecelerationSwimming);
		Velocity.Z += GetGravityZ() * deltaTime * (1.f - NetBuoyancy);
	}

	FVector Adjusted = Velocity * deltaTime;
	FHitResult Hit(1.f);
	float remainingTime = deltaTime * Swim(Adjusted, Hit);

	//may have left water - if so, script might have set new physics mode
	if ( !IsSwimming() )
	{
		StartNewPhysics(remainingTime, Iterations);
		return;
	}

	if ( Hit.Time < 1.f && CharacterOwner)
	{
		float stepZ = CharacterOwner->GetActorLocation().Z;
		FVector RealVelocity = Velocity;
		Velocity.Z = 1.f;	// HACK: since will be moving up, in case pawn leaves the water
		StepUp(-1.f*Hit.ImpactNormal, Adjusted * (1.f - Hit.Time), Hit);
		//may have left water - if so, script might have set new physics mode
		if ( !IsSwimming() )
		{
			StartNewPhysics(remainingTime, Iterations);
			return;
		}
		Velocity = RealVelocity;
		OldLocation.Z = CharacterOwner->GetActorLocation().Z + (OldLocation.Z - stepZ);
	}

	if( !HasRootMotion() && !bJustTeleported && (remainingTime < deltaTime) && CharacterOwner )
	{
		bool bWaterJump = !GetPhysicsVolume()->bWaterVolume;
		float velZ = Velocity.Z;
		Velocity = (CharacterOwner->GetActorLocation() - OldLocation) / (deltaTime - remainingTime);
		if (bWaterJump)
		{
			Velocity.Z = velZ;
		}
	}

	if ( !GetPhysicsVolume()->bWaterVolume && IsSwimming() )
	{
		SetMovementMode(MOVE_Falling); //in case script didn't change it (w/ zone change)
	}

	//may have left water - if so, script might have set new physics mode
	if ( !IsSwimming() )
	{
		StartNewPhysics(remainingTime, Iterations);
	}
}


void UCharacterMovementComponent::StartSwimming(FVector OldLocation, FVector OldVelocity, float timeTick, float remainingTime, int32 Iterations)
{
	if( !HasRootMotion() && !bJustTeleported )
	{
		if ( timeTick > 0.f )
		{
			Velocity = (CharacterOwner->GetActorLocation() - OldLocation)/timeTick; //actual average velocity
		}
		Velocity = 2.f*Velocity - OldVelocity; //end velocity has 2* accel of avg
		if (Velocity.SizeSquared() > FMath::Square(GetPhysicsVolume()->TerminalVelocity))
		{
			Velocity = Velocity.SafeNormal();
			Velocity *= GetPhysicsVolume()->TerminalVelocity;
		}
	}
	FVector End = FindWaterLine(CharacterOwner->GetActorLocation(), OldLocation);
	float waterTime = 0.f;
	if (End != CharacterOwner->GetActorLocation())
	{	
		waterTime = timeTick * (End - CharacterOwner->GetActorLocation()).Size()/(CharacterOwner->GetActorLocation() - OldLocation).Size();
		remainingTime += waterTime;
		MoveUpdatedComponent(End - CharacterOwner->GetActorLocation(), CharacterOwner->GetActorRotation(), true);
	}
	if ( !HasRootMotion() && CharacterOwner && (Velocity.Z > 2.f*SWIMBOBSPEED) && (Velocity.Z < 0.f)) //allow for falling out of water
	{
		Velocity.Z = SWIMBOBSPEED - Velocity.Size2D() * 0.7f; //smooth bobbing
	}
	if ( (remainingTime > 0.01f) && (Iterations < 8) && CharacterOwner )
	{
		PhysSwimming(remainingTime, Iterations);
	}
}

float UCharacterMovementComponent::Swim(FVector Delta, FHitResult &Hit)
{
	FVector Start = CharacterOwner->GetActorLocation();
	float airTime = 0.f;
	SafeMoveUpdatedComponent(Delta, CharacterOwner->GetActorRotation(), true, Hit);

	if ( !GetPhysicsVolume()->bWaterVolume ) //then left water
	{
		FVector End = FindWaterLine(Start,CharacterOwner->GetActorLocation());
		if (End != CharacterOwner->GetActorLocation())
		{
			airTime = (End - CharacterOwner->GetActorLocation()).Size()/Delta.Size();
			if ( ((CharacterOwner->GetActorLocation() - Start) | (End - CharacterOwner->GetActorLocation())) > 0.f )
			{
				airTime = 0.f;
			}
			SafeMoveUpdatedComponent(End - CharacterOwner->GetActorLocation(), CharacterOwner->GetActorRotation(), true, Hit);
		}
	}
	return airTime;
}

FVector UCharacterMovementComponent::FindWaterLine(FVector InWater, FVector OutofWater)
{
	FVector Result = OutofWater;

	TArray<FHitResult> Hits;
	static const FName NAME_FindWaterLine = FName(TEXT("FindWaterLine"));
	GetWorld()->LineTraceMulti(Hits, OutofWater, InWater, UpdatedComponent->GetCollisionObjectType(), FCollisionQueryParams(NAME_FindWaterLine, true, CharacterOwner));

	for( int32 HitIdx = 0; HitIdx < Hits.Num(); HitIdx++ )
	{
		const FHitResult& Check = Hits[HitIdx];
		if ( !CharacterOwner->IsOwnedBy(Check.GetActor()) && !Check.Component.Get()->IsWorldGeometry() )
		{
			APhysicsVolume *W = Cast<APhysicsVolume>(Check.GetActor());
			if ( W && W->bWaterVolume )
			{
				FVector Dir = (InWater - OutofWater).SafeNormal();
				Result = Check.Location;
				if ( W == GetPhysicsVolume() )
					Result += 0.1f * Dir;
				else
					Result -= 0.1f * Dir;
				break;
			}
		}
	}

	return Result;
}

void UCharacterMovementComponent::NotifyJumpApex() 
{
	if( CharacterOwner )
	{
		CharacterOwner->NotifyJumpApex();
	}
}


void UCharacterMovementComponent::PhysFalling(float deltaTime, int32 Iterations)
{
	// Bound final 2d portion of velocity
	const float Speed2d = Velocity.Size2D();
	float BoundSpeed = FMath::Max(Speed2d, GetModifiedMaxSpeed());

	//bound acceleration, falling object has minimal ability to impact acceleration
	FVector RealAcceleration = Acceleration;
	FHitResult Hit(1.f);

	Acceleration.Z = 0.f;
	if( !HasRootMotion() )
	{
		// test for slope to avoid using air control to climb walls
		float TickAirControl = AirControl;
		if ( TickAirControl > 0.0f && Acceleration.SizeSquared() > 0.f )
		{
			const float TestWalkTime = FMath::Max(deltaTime, 0.05f);
			const FVector TestWalk = ((TickAirControl * GetModifiedMaxAcceleration() * Acceleration.SafeNormal() + GetGravityZ()) * TestWalkTime + Velocity) * TestWalkTime;
			if(!TestWalk.IsZero())
			{
				static const FName FallingTraceParamsTag = FName(TEXT("PhysFalling"));
				FHitResult Result(1.f);
				FCollisionQueryParams CapsuleQuery(FallingTraceParamsTag, false, CharacterOwner);
				FCollisionResponseParams ResponseParam;
				InitCollisionParams(CapsuleQuery, ResponseParam);
				const FVector PawnLocation = CharacterOwner->GetActorLocation();
				const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();
				const bool bHit = GetWorld()->SweepSingle( Result, PawnLocation, PawnLocation + TestWalk, FQuat::Identity, CollisionChannel, GetPawnCapsuleCollisionShape(SHRINK_None), CapsuleQuery, ResponseParam );
				if (bHit)
				{
					// Only matters if we can't walk there
					if (!IsValidLandingSpot(Result.Location, Result))
					{
						TickAirControl = 0.f;
					}
				}
			}
		}

		float MaxAccel = GetModifiedMaxAcceleration() * TickAirControl;

		// Boost maxAccel to increase player's control when falling
		if( (Speed2d < 10.f) && (TickAirControl > 0.f) && (TickAirControl <= 0.05f) ) //allow initial burst
		{
			MaxAccel = MaxAccel + (10.f - Speed2d)/deltaTime;
		}
		
		Acceleration = Acceleration.ClampMaxSize(MaxAccel);
	}

	float remainingTime = deltaTime;
	float timeTick = 0.1f;

	while( (remainingTime > 0.f) && (Iterations < 8) )
	{
		Iterations++;
		timeTick = (remainingTime > 0.05f) 
			? FMath::Min(0.05f, remainingTime * 0.5f)
			: remainingTime;

		remainingTime -= timeTick;
		const FVector OldLocation = CharacterOwner->GetActorLocation();
		const FRotator PawnRotation = CharacterOwner->GetActorRotation();
		bJustTeleported = false;

		FVector OldVelocity = Velocity;

		// Apply input
		if (!HasRootMotion())
		{
			const float SavedVelZ = Velocity.Z;
			Velocity.Z = 0.f;
			CalcVelocity(timeTick, FallingLateralFriction, false, BrakingDecelerationFalling);
			Velocity.Z = SavedVelZ;
		}

		// Apply gravity
		Velocity = NewFallVelocity(Velocity, FVector(0.f,0.f,GetGravityZ()), timeTick);

		if( bNotifyApex && CharacterOwner->Controller && (Velocity.Z <= 0.f) )
		{
			// Just passed jump apex since now going down
			bNotifyApex = false;
			NotifyJumpApex();
		}

		if( !HasRootMotion() )
		{
			// make sure not exceeding acceptable speed
			Velocity = Velocity.ClampMaxSize2D(BoundSpeed);
		}

		FVector Adjusted = 0.5f*(OldVelocity + Velocity) * timeTick;  
		SafeMoveUpdatedComponent( Adjusted, PawnRotation, true, Hit);
		
		if( !CharacterOwner || CharacterOwner->IsPendingKill() )
		{
			return;
		}
		
		if ( IsSwimming() ) //just entered water
		{
			remainingTime = remainingTime + timeTick * (1.f - Hit.Time);
			StartSwimming(OldLocation, OldVelocity, timeTick, remainingTime, Iterations);
			return;
		}
		else if ( Hit.Time < 1.f )
		{
			if (IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
			{
				remainingTime += timeTick * (1.f - Hit.Time);
				if (!bJustTeleported && (Hit.Time > 0.1f) && (Hit.Time * timeTick > 0.003f) )
				{
					Velocity = (CharacterOwner->GetActorLocation() - OldLocation)/(timeTick * Hit.Time);
				}
				ProcessLanded(Hit, remainingTime, Iterations);
				return;
			}
			else
			{
				HandleImpact(Hit, deltaTime, Adjusted);
				
				if( !RealAcceleration.IsZero() )
				{
					// If we've changed physics mode, abort.
					if( !CharacterOwner || CharacterOwner->IsPendingKill() || !IsFalling() )
					{
						return;
					}
				}

				const FVector OldHitNormal = Hit.Normal;
				const FVector OldHitImpactNormal = Hit.ImpactNormal;
				FVector Delta = ComputeSlideVector(Adjusted, 1.f - Hit.Time, OldHitNormal, Hit);

				if ((Delta | Adjusted) > 0.f)
				{
					SafeMoveUpdatedComponent( Delta, PawnRotation, true, Hit);
					if (Hit.Time < 1.f) //hit second wall
					{
						if (IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit))
						{
							remainingTime = 0.f;
							ProcessLanded(Hit, remainingTime, Iterations);
							return;
						}

						HandleImpact(Hit, timeTick, Delta);

						// If we've changed physics mode, abort.
						if( !CharacterOwner || CharacterOwner->IsPendingKill() || !IsFalling() )
						{
							return;
						}

 						TwoWallAdjust(Delta, Hit, OldHitNormal);

						// bDitch=true means that pawn is straddling two slopes, neither of which he can stand on
						bool bDitch = ( (OldHitImpactNormal.Z > 0.f) && (Hit.ImpactNormal.Z > 0.f) && (FMath::Abs(Delta.Z) <= KINDA_SMALL_NUMBER) && ((Hit.ImpactNormal | OldHitImpactNormal) < 0.f) );
						SafeMoveUpdatedComponent( Delta, PawnRotation, true, Hit);
						if ( Hit.Time == 0 )
						{
							// if we are stuck then try to side step
							FVector SideDelta = (OldHitNormal + Hit.ImpactNormal).SafeNormal2D();
							if ( SideDelta.IsNearlyZero() )
							{
								SideDelta = FVector(OldHitNormal.Y, -OldHitNormal.X, 0).SafeNormal();
							}
							SafeMoveUpdatedComponent( SideDelta, PawnRotation, true, Hit);
						}
							
						if ( bDitch || IsValidLandingSpot(UpdatedComponent->GetComponentLocation(), Hit) || Hit.Time == 0  )
						{
							remainingTime = 0.f;
							ProcessLanded(Hit, remainingTime, Iterations);
							return;
						}
						else if (GetPerchRadiusThreshold() > 0.f && Hit.Time == 1.f && OldHitImpactNormal.Z >= WalkableFloorZ)
						{
							// We might be in a virtual 'ditch' within our perch radius. This is rare.
							const FVector PawnLocation = CharacterOwner->GetActorLocation();
							const float ZMovedDist = FMath::Abs(PawnLocation.Z - OldLocation.Z);
							const float MovedDist2DSq = (PawnLocation - OldLocation).SizeSquared2D();
							if (ZMovedDist <= 0.2f * timeTick && MovedDist2DSq <= 4.f * timeTick)
							{
								Velocity.X += 0.25f * GetMaxSpeed() * (FMath::FRand() - 0.5f);
								Velocity.Y += 0.25f * GetMaxSpeed() * (FMath::FRand() - 0.5f);
								Velocity.Z = FMath::Max<float>(JumpZVelocity * 0.25f, 1.f);
								Delta = Velocity * timeTick;
								SafeMoveUpdatedComponent(Delta, PawnRotation, true, Hit);
							}
						}
					}
				}

				// Calculate average velocity based on actual movement after considering collisions
				if (!bJustTeleported)
				{
					// Use average velocity for XY movement (no acceleration except for air control in those axes), but want actual velocity in Z axis
					const float OldVelZ = OldVelocity.Z;
					OldVelocity = (CharacterOwner->GetActorLocation() - OldLocation)/timeTick;
					OldVelocity.Z = OldVelZ;
				}
			}
		}

		if (!HasRootMotion() && !bJustTeleported && MovementMode != MOVE_None )
		{
			// refine the velocity by figuring out the average actual velocity over the tick, and then the final velocity.
			// This particularly corrects for situations where level geometry affected the fall.
			Velocity = (CharacterOwner->GetActorLocation() - OldLocation)/timeTick; //actual average velocity
			if( (Velocity.Z < OldVelocity.Z) || (OldVelocity.Z >= 0.f) )
			{
				Velocity = 2.f*Velocity - OldVelocity; //end velocity has 2* accel of avg
			}

			if (Velocity.SizeSquared2D() <= KINDA_SMALL_NUMBER * 10.f)
			{
				Velocity.X = 0.f;
				Velocity.Y = 0.f;
			}

			Velocity = Velocity.ClampMaxSize(GetPhysicsVolume()->TerminalVelocity);
		}
	}

	Acceleration = RealAcceleration;
}


bool UCharacterMovementComponent::CheckLedgeDirection(const FVector& OldLocation, const FVector& SideStep, const FVector& GravDir)
{
	FVector SideDest = OldLocation + SideStep;
	static const FName CheckLedgeDirectionName(TEXT("CheckLedgeDirection"));
	FCollisionQueryParams CapsuleParams(CheckLedgeDirectionName, false, CharacterOwner);
	FCollisionResponseParams ResponseParam;
	InitCollisionParams(CapsuleParams, ResponseParam);
	FCollisionShape CapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_None);
	const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();
	FHitResult Result(ForceInit);
	GetWorld()->SweepSingle(Result, OldLocation, SideDest, FQuat::Identity, CollisionChannel, CapsuleShape, CapsuleParams, ResponseParam);

	if ( (Result.Time == 1.f) || IsWalkable(Result) )
	{
		if ( Result.Time == 1.f )
		{
			GetWorld()->SweepSingle(Result, SideDest, SideDest + GravDir * (MaxStepHeight + LedgeCheckThreshold), FQuat::Identity, CollisionChannel, CapsuleShape, CapsuleParams, ResponseParam);
		}
		if ( (Result.Time < 1.f) && IsWalkable(Result) )
		{
			return true;
		}
	}
	return false;
}


FVector UCharacterMovementComponent::GetLedgeMove(const FVector& OldLocation, const FVector& Delta, const FVector& GravDir)
{
	// We have a ledge!
	if ( !CharacterOwner )
	{
		return FVector::ZeroVector;
	}

	// check which direction ledge goes
	float DesiredDistSq = Delta.SizeSquared();

	if ( DesiredDistSq > 0.f )
	{
		FVector SideDir(Delta.Y, -1.f * Delta.X, 0.f);
		
		// try left
		if ( CheckLedgeDirection(OldLocation, SideDir, GravDir) )
		{
			return SideDir;
		}

		// try right
		SideDir *= -1.f;
		if ( CheckLedgeDirection(OldLocation, SideDir, GravDir) )
		{
			return SideDir;
		}
	}
	return FVector::ZeroVector;
}


bool UCharacterMovementComponent::CanWalkOffLedges() const
{
	if (!bCanWalkOffLedgesWhenCrouching && IsCrouching())
	{
		return false;
	}	

	return bCanWalkOffLedges;
}

bool UCharacterMovementComponent::CheckFall(FHitResult &Hit, FVector Delta, FVector subLoc, float remainingTime, float timeTick, int32 Iterations, bool bMustJump)
{
	if (!CharacterOwner)
	{
		return false;
	}

	if (bMustJump || CanWalkOffLedges())
	{
		StartFalling(Iterations, remainingTime, timeTick, Delta, subLoc);
		return true;
	}
	return false;
}

void UCharacterMovementComponent::StartFalling(int32 Iterations, float remainingTime, float timeTick, const FVector& Delta, const FVector& subLoc)
{
	// start falling 
	const float DesiredDist = Delta.Size();
	const float ActualDist = (CharacterOwner->GetActorLocation() - subLoc).Size2D();
	remainingTime = (DesiredDist == 0.f) 
					? 0.f
					: remainingTime + timeTick * (1.f - FMath::Min(1.f,ActualDist/DesiredDist));

	Velocity.Z = 0.f;			
	if ( IsMovingOnGround() )
	{
		// This is to catch cases where the first frame of PIE is executed, and the
		// level is not yet visible. In those cases, the player will fall out of the
		// world... So, don't set MOVE_Falling straight away.
		if ( !GIsEditor || (GetWorld()->HasBegunPlay() && (GetWorld()->GetTimeSeconds() >= 1.f)) )
		{
			SetMovementMode(MOVE_Falling); //default behavior if script didn't change physics
		}
		else
		{
			// Make sure that the floor check code continues processing during this delay.
			bForceNextFloorCheck = true;
		}
	}
	StartNewPhysics(remainingTime,Iterations);
}


void UCharacterMovementComponent::RevertMove(const FVector& OldLocation, UPrimitiveComponent* OldBase, const FVector& PreviousBaseLocation, const FFindFloorResult& OldFloor, bool bFailMove)
{
	//UE_LOG(LogCharacterMovement, Log, TEXT("RevertMove from %f %f %f to %f %f %f"), CharacterOwner->Location.X, CharacterOwner->Location.Y, CharacterOwner->Location.Z, OldLocation.X, OldLocation.Y, OldLocation.Z);
	UpdatedComponent->SetWorldLocation(OldLocation, false);
	
	//UE_LOG(LogCharacterMovement, Log, TEXT("Now at %f %f %f"), CharacterOwner->Location.X, CharacterOwner->Location.Y, CharacterOwner->Location.Z);
	bJustTeleported = false;
	// if our previous base couldn't have moved or changed in any physics-affecting way, restore it
	if (OldBase && 
		(OldBase->IsWorldGeometry() ||
		 (OldBase->Mobility == EComponentMobility::Static) ||
		 (OldBase->GetOwner() && OldBase->GetOwner()->GetActorLocation() == PreviousBaseLocation)
		)
	   )
	{
		CurrentFloor = OldFloor;
		SetBase(OldBase);
	}
	else
	{
		SetBase(NULL);
	}

	if ( bFailMove )
	{
		// end movement now
		Velocity = FVector::ZeroVector;
		Acceleration = FVector::ZeroVector;
		//UE_LOG(LogCharacterMovement, Log, TEXT("%s FAILMOVE RevertMove"), *CharacterOwner->GetName());

		if (PathFollowingComp.IsValid())
		{
			PathFollowingComp->AbortMove(TEXT("RevertMove"));
		}
	}
}


FVector UCharacterMovementComponent::ComputeGroundMovementDelta(const FVector& Delta, const FHitResult& RampHit, const bool bHitFromLineTrace) const
{
	const FVector FloorNormal = RampHit.ImpactNormal;
	const FVector ContactNormal = RampHit.Normal;

	if (FloorNormal.Z < (1.f - KINDA_SMALL_NUMBER) && FloorNormal.Z > KINDA_SMALL_NUMBER && ContactNormal.Z > KINDA_SMALL_NUMBER && !bHitFromLineTrace && IsWalkable(RampHit))
	{
		const float FloorDotDelta = (FloorNormal | Delta);
		FVector RampMovement(Delta.X, Delta.Y, -FloorDotDelta / FloorNormal.Z);
		
		if (bMaintainHorizontalGroundVelocity)
		{
			return RampMovement;
		}
		else
		{
			return RampMovement.SafeNormal() * Delta.Size();
		}
	}

	return Delta;
}


void UCharacterMovementComponent::MoveAlongFloor(const FVector& InVelocity, const float DeltaSeconds, FStepDownResult* OutStepDownResult)
{
	const FVector Delta = FVector(InVelocity.X, InVelocity.Y, 0.f) * DeltaSeconds;
	
	if (!CurrentFloor.IsWalkableFloor())
	{
		return;
	}

	FHitResult Hit(1.f);
	FVector RampVector = ComputeGroundMovementDelta(Delta, CurrentFloor.HitResult, CurrentFloor.bLineTrace);
	SafeMoveUpdatedComponent(RampVector, CharacterOwner->GetActorRotation(), true, Hit);
	if (Hit.bStartPenetrating)
	{
		UE_LOG(LogCharacterMovement, Log, TEXT("%s is stuck and failed to move!"), *CharacterOwner->GetName());
		
		// Don't update velocity based on our (failed) change in position this update since we're stuck.
		bJustTeleported = true;
	}

	if (Hit.IsValidBlockingHit())
	{
		// See if we impacted something (most likely another ramp, but possibly a barrier). Try to slide along it as well.
		float TimeApplied = Hit.Time;
		if ((Hit.Time > 0.f) && (Hit.Normal.Z > KINDA_SMALL_NUMBER) && IsWalkable(Hit))
		{
			const float PreSlideTimeRemaining = 1.f - Hit.Time;
			RampVector = ComputeGroundMovementDelta(Delta * PreSlideTimeRemaining, Hit, false);
			SafeMoveUpdatedComponent(RampVector, CharacterOwner->GetActorRotation(), true, Hit);

			const float SecondHitPercent = Hit.Time * (1.f - TimeApplied);
			TimeApplied = FMath::Clamp(TimeApplied + SecondHitPercent, 0.f, 1.f);
		}

		if (Hit.IsValidBlockingHit())
		{
			if (CanStepUp(Hit) || (CharacterOwner->GetMovementBase() != NULL && CharacterOwner->GetMovementBase()->GetOwner() == Hit.GetActor()))
			{
				// hit a barrier, try to step up
				const FVector GravDir(0.f, 0.f, -1.f);
				if (!StepUp(GravDir, Delta * (1.f - TimeApplied), Hit, OutStepDownResult))
				{
					UE_LOG(LogCharacterMovement, Verbose, TEXT("- StepUp (ImpactNormal %s, Normal %s"), *Hit.ImpactNormal.ToString(), *Hit.Normal.ToString());
					HandleImpact(Hit, DeltaSeconds, Delta);
					SlideAlongSurface(Delta, 1.f - TimeApplied, Hit.Normal, Hit, true);
				}
				else
				{
					// Don't recalculate velocity based on this height adjustment, if considering vertical adjustments.
					UE_LOG(LogCharacterMovement, Verbose, TEXT("+ StepUp (ImpactNormal %s, Normal %s"), *Hit.ImpactNormal.ToString(), *Hit.Normal.ToString());
					bJustTeleported |= !bMaintainHorizontalGroundVelocity;
				}
			}
			else if ( Hit.Component.IsValid() && !Hit.Component.Get()->CanBeBaseForCharacter(CharacterOwner) )
			{
				HandleImpact(Hit, DeltaSeconds, Delta);
				SlideAlongSurface(Delta, 1.f - TimeApplied, Hit.Normal, Hit, true);
			}
		}
	}
}


void UCharacterMovementComponent::MaintainHorizontalGroundVelocity()
{
	if (Velocity.Z != 0.f)
	{
		if (bMaintainHorizontalGroundVelocity)
		{
			// Ramp movement already maintained the velocity, so we just want to remove the vertical component.
			Velocity.Z = 0.f;
		}
		else
		{
			// Rescale velocity to be horizontal but maintain magnitude of last update.
			Velocity = Velocity.SafeNormal2D() * Velocity.Size();
		}
	}
}


void UCharacterMovementComponent::PhysWalking(float deltaTime, int32 Iterations)
{
	if( (!CharacterOwner || !CharacterOwner->Controller) && !bRunPhysicsWithNoController && !HasRootMotion() )
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	if (!UpdatedComponent->IsCollisionEnabled())
	{
		SetMovementMode(MOVE_Walking);
		return;
	}

	// Ensure velocity is horizontal.
	MaintainHorizontalGroundVelocity();

	//bound acceleration
	Acceleration.Z = 0.f;
	if( !HasRootMotion() )
	{
		CalcVelocity(deltaTime, GroundFriction, false, BrakingDecelerationWalking);
	}

	FVector DesiredMove = Velocity;
	DesiredMove.Z = 0.f;

	//Perform the move
	const FVector OldLocation = UpdatedComponent->GetComponentLocation();
	const FFindFloorResult OldFloor = CurrentFloor;
	UPrimitiveComponent* const OldBase = CharacterOwner->GetMovementBase();
	const FVector PreviousBaseLocation = (OldBase != NULL) ? OldBase->GetComponentLocation() : FVector::ZeroVector;
	bJustTeleported = false;
	bool bCheckedFall = false;
	float remainingTime = deltaTime;

	while ( (remainingTime > 0.f) && (Iterations < 8) && (CharacterOwner->Controller || bRunPhysicsWithNoController || HasRootMotion()) )
	{
		Iterations++;
		// subdivide moves to be no longer than 0.05 seconds
		const float timeTick = (remainingTime > 0.05f) ? FMath::Min(0.05f, remainingTime * 0.5f) : remainingTime;
		remainingTime -= timeTick;
		const FVector Delta = timeTick * DesiredMove;
		const FVector subLoc = CharacterOwner->GetActorLocation();
		const bool bZeroDelta = Delta.IsNearlyZero();
		FStepDownResult StepDownResult;

		if ( bZeroDelta )
		{
			remainingTime = 0.f;
		}
		else
		{
			// @todo hunting down NaN TTP 304692
			checkf( !Delta.ContainsNaN(), TEXT("PhysWalking: NewTransform contains NaN (%s: %s)\n%s"), *GetPathNameSafe(this), *GetPathNameSafe(GetOuter()), *Delta.ToString() );
			// @todo hunting down NaN TTP 304692

			// try to move forward
			MoveAlongFloor(DesiredMove, timeTick, &StepDownResult);

			if ( IsFalling() )
			{
				// pawn decided to jump up
				const float ActualDist = (CharacterOwner->GetActorLocation() - subLoc).Size2D();
				const float DesiredDist = Delta.Size();
				remainingTime += timeTick * (1 - FMath::Min(1.f,ActualDist/DesiredDist));
				StartNewPhysics(remainingTime,Iterations);
				return;
			}
			else if ( IsSwimming() ) //just entered water
			{
				StartSwimming(OldLocation, Velocity, timeTick, remainingTime, Iterations);
				return;
			}
		}

		// Update floor.
		// StepUp might have already done it for us.
		if (StepDownResult.bComputedFloor)
		{
			CurrentFloor = StepDownResult.FloorResult;
		}
		else
		{
			FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, bZeroDelta, NULL);
		}

		// check for ledges here
		const bool bCheckLedges = !CanWalkOffLedges();
		if ( bCheckLedges && !CurrentFloor.IsWalkableFloor() )
		{
			// calculate possible alternate movement
			const FVector GravDir = FVector(0.f,0.f,-1.f);
			FVector NewDelta = GetLedgeMove(OldLocation, Delta, GravDir);
			if ( !NewDelta.IsZero() )
			{
				// first revert this move
				RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, false);

				// @todo hunting down NaN TTP 304692
				check (timeTick!=0.f);
				// redo move using NewDelta
				DesiredMove = NewDelta/timeTick;
				remainingTime += timeTick;
			}
			else
			{
				// see if it is OK to jump
				// @todo collision : only thing that can be problem is that oldbase has world collision on
				bool bMustJump = bZeroDelta || (OldBase == NULL || ((!OldBase->IsCollisionEnabled()) && !OldBase->IsWorldGeometry()));
				if ( (bMustJump || !bCheckedFall) && CheckFall(CurrentFloor.HitResult, Delta, subLoc, remainingTime, timeTick, Iterations, bMustJump) )
				{
					return;
				}
				bCheckedFall = true;

				// revert this move
				//UE_LOG(LogCharacterMovement, Log, TEXT("%s REVERT MOVE 1"), *CharacterOwner->GetName());
				RevertMove(OldLocation, OldBase, PreviousBaseLocation, OldFloor, true);
				remainingTime = 0.f;
			}
		}
		else
		{
			// Validate the floor check
			if (CurrentFloor.IsWalkableFloor())
			{
				const bool bBaseChanged = (CurrentFloor.HitResult.Component != CharacterOwner->GetMovementBase());
				if (bBaseChanged || CurrentFloor.FloorDist > MAX_FLOOR_DIST)
				{
					if ( ShouldCatchAir(OldFloor.HitResult.ImpactNormal, CurrentFloor.HitResult.ImpactNormal) )
					{
						StartFalling(Iterations, remainingTime, timeTick, Delta, subLoc);
						return;
					}
					else
					{
						AdjustFloorHeight();
						if (bBaseChanged)
						{
							SetBase(CurrentFloor.HitResult.Component.Get());
						}
					}
				}
				else if (CurrentFloor.FloorDist < MIN_FLOOR_DIST)
				{
					AdjustFloorHeight();
				}
			}
			else if (CurrentFloor.HitResult.bStartPenetrating && remainingTime <= 0.f)
			{
				// The floor check failed because it started in penetration
				// We do not want to try to move downward because the downward sweep failed, rather we'd like to try to pop out of the floor.
				FHitResult Hit(CurrentFloor.HitResult);
				Hit.TraceEnd = Hit.TraceStart + FVector(0.f, 0.f, MAX_FLOOR_DIST);
				const FVector RequestedAdjustment = GetPenetrationAdjustment(Hit);
				ResolvePenetration(RequestedAdjustment, Hit, CharacterOwner->GetActorRotation());
			}

			// check if just entered water
			if ( IsSwimming() )
			{
				StartSwimming(OldLocation, Velocity, timeTick, remainingTime, Iterations);
				return;
			}

			// See if we need to start falling.
			if (!CurrentFloor.IsWalkableFloor() && !CurrentFloor.HitResult.bStartPenetrating)
			{
				const bool bMustJump = bJustTeleported || bZeroDelta || (OldBase == NULL || (!OldBase->IsCollisionEnabled() && !OldBase->IsWorldGeometry()));
				if ((bMustJump || !bCheckedFall) && CheckFall(CurrentFloor.HitResult, Delta, subLoc, remainingTime, timeTick, Iterations, bMustJump) )
				{
					return;
				}
				bCheckedFall = true;
			}
		}
	}

	// Allow overlap events and such to change physics state and velocity
	if( IsMovingOnGround() )
	{
		// Make velocity reflect actual move
		if( !bJustTeleported && !HasRootMotion() )
		{
			Velocity = (CharacterOwner->GetActorLocation() - OldLocation) / deltaTime;
		}

		MaintainHorizontalGroundVelocity();
	}
}


void UCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{

}


bool UCharacterMovementComponent::ShouldCatchAir(const FVector& OldFloor, const FVector& Floor)
{
	return false;
}

void UCharacterMovementComponent::AdjustFloorHeight()
{
	// If we have a floor check that hasn't hit anything, don't adjust height.
	if (!CurrentFloor.bBlockingHit)
	{
		return;
	}

	const float OldFloorDist = CurrentFloor.FloorDist;
	if (CurrentFloor.bLineTrace && OldFloorDist < MIN_FLOOR_DIST)
	{
		// This would cause us to scale unwalkable walls
		return;
	}

	// Move up or down to maintain floor height.
	if (OldFloorDist < MIN_FLOOR_DIST || OldFloorDist > MAX_FLOOR_DIST)
	{
		FHitResult AdjustHit(1.f);
		const float InitialZ = UpdatedComponent->GetComponentLocation().Z;
		const float AvgFloorDist = (MIN_FLOOR_DIST + MAX_FLOOR_DIST) * 0.5f;
		const float MoveDist = AvgFloorDist - OldFloorDist;
		SafeMoveUpdatedComponent( FVector(0.f,0.f,MoveDist), CharacterOwner->GetActorRotation(), true, AdjustHit );
		UE_LOG(LogCharacterMovement, VeryVerbose, TEXT("Adjust floor height %.3f (Hit = %d)"), MoveDist, AdjustHit.bBlockingHit);

		if (!AdjustHit.IsValidBlockingHit())
		{
			CurrentFloor.FloorDist += MoveDist;
		}
		else if (MoveDist > 0.f)
		{
			// If moving up, use the actual impact location, not the pulled back time/location.
			const float FloorDistTime = FMath::Abs((InitialZ - AdjustHit.Location.Z) / MoveDist);
			CurrentFloor.FloorDist += MoveDist * FloorDistTime;
		}
		else
		{
			check(MoveDist < 0.f);
			const float CurrentZ = UpdatedComponent->GetComponentLocation().Z;
			CurrentFloor.FloorDist = CurrentZ - AdjustHit.Location.Z;
			if (IsWalkable(AdjustHit))
			{
				CurrentFloor.SetFromSweep(AdjustHit, CurrentFloor.FloorDist, true);
			}
		}

		// Don't recalculate velocity based on this height adjustment, if considering vertical adjustments.
		// Also avoid it if we moved out of penetration
		bJustTeleported |= !bMaintainHorizontalGroundVelocity || (OldFloorDist < 0.f);
	}
}

float UCharacterMovementComponent::GetMaxSpeedModifier() const
{
	if (CharacterOwner)
	{
		return IsCrouching() && IsMovingOnGround() ? CrouchedSpeedMultiplier : 1.f;
	}
	else
	{
		return 1.0f;
	}
}

void UCharacterMovementComponent::StopActiveMovement() 
{ 
	Acceleration = FVector::ZeroVector; 
	bHasRequestedVelocity = false;
	RequestedVelocity = FVector::ZeroVector;
}

void UCharacterMovementComponent::ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations)
{
	if( CharacterOwner && CharacterOwner->NotifyLanded(Hit) )
	{
		CharacterOwner->Landed(Hit);
	}
	if( IsFalling() )
	{
		SetPostLandedPhysics(Hit);
	}

	StartNewPhysics(remainingTime, Iterations);
}

void UCharacterMovementComponent::SetPostLandedPhysics(const FHitResult& Hit)
{
	if( CharacterOwner )
	{
		if ( GetPhysicsVolume()->bWaterVolume )
		{
			SetMovementMode(MOVE_Swimming);
		}
		else
		{
			SetMovementMode(MOVE_Walking);
		}
	}
	else
	{
		DisableMovement();
	}
}


/** Internal. */
FVector CharAngularVelocity(FRotator const& OldRot, FRotator const& NewRot, float DeltaTime)
{
	FVector RetAngVel(0.f);

	if (OldRot != NewRot)
	{
		float const InvDeltaTime = 1.f / DeltaTime;
		FQuat const DeltaQRot = (NewRot - OldRot).Quaternion();

		FVector Axis; 
		float Angle;
		DeltaQRot.ToAxisAndAngle(Axis, Angle);

		RetAngVel = Axis * Angle * InvDeltaTime;
		check(!RetAngVel.ContainsNaN());
	}

	return RetAngVel;
}

FRotator UCharacterMovementComponent::GetDeltaRotation(float DeltaTime)
{
	return (RotationRate * DeltaTime);
}

FRotator UCharacterMovementComponent::ComputeOrientToMovementRotation(const FRotator& CurrentRotation, float DeltaTime, FRotator& DeltaRotation)
{
	// Do nothing if not accelerating.
	if (Acceleration.SizeSquared() < KINDA_SMALL_NUMBER)
	{
		return CurrentRotation;
	}

	// Rotate toward direction of acceleration.
	return Acceleration.SafeNormal().Rotation();
}

void UCharacterMovementComponent::PhysicsRotation(float DeltaTime)
{
	if ( !CharacterOwner || !CharacterOwner->Controller )
	{
		return;
	}

	const FRotator CurrentRotation = CharacterOwner->GetActorRotation();
	FRotator DeltaRot = GetDeltaRotation(DeltaTime);
	FRotator DesiredRotation = CurrentRotation;

	if (bOrientRotationToMovement)
	{
		DesiredRotation = ComputeOrientToMovementRotation(CurrentRotation, DeltaTime, DeltaRot);
	}
	else if (bUseControllerDesiredRotation)
	{
		DesiredRotation = CharacterOwner->Controller->GetDesiredRotation();
	}
	else
	{
		return;
	}

	// Always remain vertical when walking or falling.
	if( IsMovingOnGround() || IsFalling() )
	{
		DesiredRotation.Pitch = 0;
		DesiredRotation.Roll = 0;
	}

	if( CurrentRotation.GetDenormalized().Equals(DesiredRotation.GetDenormalized(), 0.01f) )
	{
		return;
	}

	// Accumulate a desired new rotation.
	FRotator NewRotation = CurrentRotation;	

	//YAW
	if( DesiredRotation.Yaw != CurrentRotation.Yaw )
	{
		NewRotation.Yaw = FMath::FixedTurn(CurrentRotation.Yaw, DesiredRotation.Yaw, DeltaRot.Yaw);
	}

	// PITCH
	if( DesiredRotation.Pitch != CurrentRotation.Pitch )
	{
		NewRotation.Pitch = FMath::FixedTurn(CurrentRotation.Pitch, DesiredRotation.Pitch, DeltaRot.Pitch);
	}

	// ROLL
	if( DesiredRotation.Roll != CurrentRotation.Roll )
	{
		NewRotation.Roll = FMath::FixedTurn(CurrentRotation.Roll, DesiredRotation.Roll, DeltaRot.Roll);
	}

	//UpdatedComponent->AngularVelocity = CharAngularVelocity( CurrentRotation, NewRotation, deltaTime );

	// Set the new rotation.
	if( !NewRotation.Equals(CurrentRotation.GetDenormalized(), 0.01f) )
	{
		MoveUpdatedComponent( FVector::ZeroVector, NewRotation, true );
	}
}

bool UPrimitiveComponent::CanBeBaseForCharacter(APawn* Pawn) const
{
	if ( CanBeCharacterBase != ECB_Owner )
	{
		return CanBeCharacterBase == ECB_Yes;
	}
	else
	{	
		const AActor* Owner = GetOwner();
		return Owner && Owner->CanBeBaseForCharacter(Pawn);
	}
}


void UCharacterMovementComponent::PhysicsVolumeChanged( APhysicsVolume* NewVolume )
{
	if ( !CharacterOwner )
	{
		return;
	}
	if ( NewVolume && NewVolume->bWaterVolume )
	{
		// just entered water
		if ( !CanEverSwim() )
		{
			// AI needs to stop any current moves
			if (PathFollowingComp.IsValid())
			{
				PathFollowingComp->AbortMove(TEXT("water"));
			}			
		}
		else if ( !IsSwimming() )
		{
			SetMovementMode(MOVE_Swimming);
		}
	}
	else if ( IsSwimming() )
	{
		// just left the water - check if should jump out
		SetMovementMode(MOVE_Falling);
		FVector JumpDir(0.f);
		FVector WallNormal(0.f);
		if ( Acceleration.Z > 0.f && ShouldJumpOutOfWater(JumpDir)
			&& ((JumpDir | Acceleration) > 0.f) && CheckWaterJump(JumpDir, WallNormal) ) 
		{
			JumpOutOfWater(WallNormal);
			Velocity.Z = OutofWaterZ; //set here so physics uses this for remainder of tick
		}
	}
}


bool UCharacterMovementComponent::ShouldJumpOutOfWater(FVector& JumpDir)
{
	AController* OwnerController = CharacterOwner->GetController();
	if (OwnerController)
	{
		const FRotator ControllerRot = OwnerController->GetControlRotation();
		if ( (Velocity.Z > 0.0f) && (ControllerRot.Pitch > JumpOutOfWaterPitch) )
		{
			// if Pawn is going up and looking up, then make him jump
			JumpDir = ControllerRot.Vector();
			return true;
		}
	}
	
	return false;
}


void UCharacterMovementComponent::JumpOutOfWater(FVector WallNormal) {}


bool UCharacterMovementComponent::CheckWaterJump(FVector CheckPoint, FVector& WallNormal)
{
	if ( !CharacterOwner || !CharacterOwner->CapsuleComponent )
	{
		return false;
	}
	// check if there is a wall directly in front of the swimming pawn
	CheckPoint.Z = 0.f;
	FVector CheckNorm = CheckPoint.SafeNormal();
	float PawnCapsuleRadius, PawnCapsuleHalfHeight;
	CharacterOwner->CapsuleComponent->GetScaledCapsuleSize(PawnCapsuleRadius, PawnCapsuleHalfHeight);
	CheckPoint = CharacterOwner->GetActorLocation() + 1.2f * PawnCapsuleRadius * CheckNorm;
	FVector Extent(PawnCapsuleRadius, PawnCapsuleRadius, PawnCapsuleHalfHeight);
	FHitResult HitInfo(1.f);
	static const FName CheckWaterJumpName(TEXT("CheckWaterJump"));
	FCollisionQueryParams CapsuleParams(CheckWaterJumpName, false, CharacterOwner);
	FCollisionResponseParams ResponseParam;
	InitCollisionParams(CapsuleParams, ResponseParam);
	FCollisionShape CapsuleShape = GetPawnCapsuleCollisionShape(SHRINK_None);
	const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();
	bool bHit = GetWorld()->SweepSingle( HitInfo, CharacterOwner->GetActorLocation(), CheckPoint, FQuat::Identity, CollisionChannel, CapsuleShape, CapsuleParams, ResponseParam);
	
	if ( HitInfo.GetActor() && !Cast<APawn>(HitInfo.GetActor()) )
	{
		// hit a wall - check if it is low enough
		WallNormal = -1.f * HitInfo.ImpactNormal;
		FVector Start = CharacterOwner->GetActorLocation();
		Start.Z += MaxOutOfWaterStepHeight;
		CheckPoint = Start + 3.2f * PawnCapsuleRadius * WallNormal;
		FCollisionQueryParams LineParams(CheckWaterJumpName, true, CharacterOwner);
		FCollisionResponseParams LineResponseParam;
		InitCollisionParams(LineParams, LineResponseParam);
		bHit = GetWorld()->LineTraceSingle( HitInfo, Start, CheckPoint, CollisionChannel, LineParams, LineResponseParam );
		// if no high obstruction, or it's a valid floor, then pawn can jump out of water
		return !bHit || IsWalkable(HitInfo);
	}
	return false;
}

void UCharacterMovementComponent::AddMomentum( FVector const& Momentum, FVector const& LocationToApplyMomentum, bool bMassIndependent )
{
	if ( CharacterOwner && !Momentum.IsZero() )
	{
		// handle scaling by mass
		FVector FinalMomentum = Momentum;
		if ( !bMassIndependent)
		{
			if (Mass > SMALL_NUMBER)
			{
				FinalMomentum = FinalMomentum / Mass;
			}
			else
			{
				UE_LOG(LogCharacterMovement, Warning, TEXT("Attempt to apply momentum to zero or negative Mass in CharacterMovement"));
			}
		}

		if ( IsMovingOnGround() && FinalMomentum.Z > 0.f )
		{
			SetMovementMode(MOVE_Falling);
		}

		if ( (Velocity.Z > GetDefault<UCharacterMovementComponent>(GetClass())->JumpZVelocity) && (FinalMomentum.Z > 0.f) )
		{
			// limit Z momentum added if already going up faster than jump (to avoid blowing character way up into the sky)
			FinalMomentum.Z *= 0.5f;
		}

		Velocity += FinalMomentum;
	}
}

void UCharacterMovementComponent::MoveSmooth(const FVector& InVelocity, const float DeltaSeconds, FStepDownResult* OutStepDownResult)
{
	FVector Delta = InVelocity * DeltaSeconds;
	if ( !CharacterOwner || Delta.IsZero() )
	{
		return;
	}

	FScopedMovementUpdate ScopedMovementUpdate(UpdatedComponent, bEnableScopedMovementUpdates ? EScopedUpdate::DeferredUpdates : EScopedUpdate::ImmediateUpdates);

	if (IsMovingOnGround())
	{
		MoveAlongFloor(InVelocity, DeltaSeconds, OutStepDownResult);
	}
	else
	{
		FHitResult Hit(1.f);
		SafeMoveUpdatedComponent(Delta, CharacterOwner->GetActorRotation(), true, Hit);
	
		if (Hit.IsValidBlockingHit())
		{
			bool bSteppedUp = false;

			if (IsFlying())
			{
				if (CanStepUp(Hit))
				{
					OutStepDownResult = NULL; // No need for a floor when not walking.
					if (FMath::Abs(Hit.ImpactNormal.Z) < 0.2f)
					{
						const FVector GravDir = FVector(0.f,0.f,-1.f);
						const FVector DesiredDir = Delta.SafeNormal();
						const float UpDown = GravDir | DesiredDir;
						if ((UpDown < 0.5f) && (UpDown > -0.2f))
						{
							bSteppedUp = StepUp(GravDir, Delta * (1.f - Hit.Time), Hit, OutStepDownResult);
						}			
					}
				}
			}
				
			// If StepUp failed, try sliding.
			if (!bSteppedUp)
			{
				SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, false);
			}
		}
	}
}


bool UCharacterMovementComponent::IsWalkable(const FHitResult& Hit) const
{
	if (!Hit.bBlockingHit)
	{
		return false;
	}

	// Never walk up vertical surfaces.
	if (Hit.ImpactNormal.Z < KINDA_SMALL_NUMBER)
	{
		return false;
	}

	float TestWalkableZ = WalkableFloorZ;

	// See if this component overrides the walkable floor z.
	const UPrimitiveComponent* HitComponent = Hit.Component.Get();
	if (HitComponent)
	{
		const FWalkableSlopeOverride& SlopeOverride = HitComponent->GetWalkableSlopeOverride();
		TestWalkableZ = SlopeOverride.ModifyWalkableFloorZ(TestWalkableZ);
	}

	// Can't walk on this surface if it is too steep.
	if (Hit.ImpactNormal.Z < TestWalkableZ)
	{
		return false;
	}

	return true;
}

void UCharacterMovementComponent::SetWalkableFloorAngle(float InWalkableFloorAngle)
{
	WalkableFloorAngle = FMath::Clamp(InWalkableFloorAngle, 0.f, 90.0f);
	WalkableFloorZ = FMath::Cos(FMath::DegreesToRadians(WalkableFloorAngle));
}

void UCharacterMovementComponent::SetWalkableFloorZ(float InWalkableFloorZ)
{
	WalkableFloorZ = FMath::Clamp(InWalkableFloorZ, 0.f, 1.f);
	WalkableFloorAngle = FMath::RadiansToDegrees(FMath::Acos(WalkableFloorZ));
}

float UCharacterMovementComponent::K2_GetWalkableFloorAngle() const
{
	return GetWalkableFloorAngle();
}

float UCharacterMovementComponent::K2_GetWalkableFloorZ() const
{
	return GetWalkableFloorZ();
}



bool UCharacterMovementComponent::IsWithinEdgeTolerance(const FVector& CapsuleLocation, const FVector& TestImpactPoint, const float CapsuleRadius) const
{
	const float DistFromCenterSq = (TestImpactPoint - CapsuleLocation).SizeSquared2D();
	const float ReducedRadiusSq = FMath::Square(FMath::Max(KINDA_SMALL_NUMBER, CapsuleRadius - SWEEP_EDGE_REJECT_DISTANCE));
	return DistFromCenterSq < ReducedRadiusSq;
}


void UCharacterMovementComponent::ComputeFloorDist(const FVector& CapsuleLocation, float LineDistance, float SweepDistance, FFindFloorResult& OutFloorResult, float SweepRadius, const FHitResult* DownwardSweepResult) const
{
	OutFloorResult.Clear();

	// No collision, no floor...
	if (!UpdatedComponent->IsCollisionEnabled())
	{
		return;
	}

	float PawnRadius, PawnHalfHeight;
	CharacterOwner->CapsuleComponent->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);

	bool bSkipSweep = false;
	if (DownwardSweepResult != NULL && DownwardSweepResult->IsValidBlockingHit())
	{
		// Only if the supplied sweep was vertical and downward.
		if ((DownwardSweepResult->TraceStart.Z > DownwardSweepResult->TraceEnd.Z) &&
			(DownwardSweepResult->TraceStart - DownwardSweepResult->TraceEnd).SizeSquared2D() <= KINDA_SMALL_NUMBER)
		{
			// Reject hits that are barely on the cusp of the radius of the capsule
			if (IsWithinEdgeTolerance(DownwardSweepResult->Location, DownwardSweepResult->ImpactPoint, PawnRadius))
			{
				// Don't try a redundant sweep, regardless of whether this sweep is usable.
				bSkipSweep = true;

				const bool bIsWalkable = IsWalkable(*DownwardSweepResult);
				const float FloorDist = (CapsuleLocation.Z - DownwardSweepResult->Location.Z);
				OutFloorResult.SetFromSweep(*DownwardSweepResult, FloorDist, bIsWalkable);
				
				if (bIsWalkable)
				{
					// Use the supplied downward sweep as the floor hit result.			
					return;
				}
			}
		}
	}

	// We require the sweep distance to be >= the line distance, otherwise the HitResult can't be interpreted as the sweep result.
	if (SweepDistance < LineDistance)
	{
		check(SweepDistance >= LineDistance);
		return;
	}

	bool bBlockingHit = false;
	FCollisionQueryParams QueryParams(NAME_None, false, CharacterOwner);
	FCollisionResponseParams ResponseParam;
	InitCollisionParams(QueryParams, ResponseParam);
	const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();

	// Sweep test
	if (!bSkipSweep && SweepDistance > 0.f && SweepRadius > 0.f)
	{
		// Use a shorter height to avoid sweeps giving weird results if we start on a surface.
		// This also allows us to adjust out of penetrations.
		const float ShrinkScale = 0.9f;
		const float ShrinkScaleOverlap = 0.6f;
		float ShrinkHeight = (PawnHalfHeight - PawnRadius) * (1.f - ShrinkScale);
		float TraceDist = SweepDistance + ShrinkHeight;

		static const FName ComputeFloorDistName(TEXT("ComputeFloorDistSweep"));
		QueryParams.TraceTag = ComputeFloorDistName;
		FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(SweepRadius, PawnHalfHeight - ShrinkHeight);

		FHitResult Hit(1.f);
		bBlockingHit = GetWorld()->SweepSingle(Hit, CapsuleLocation, CapsuleLocation + FVector(0.f,0.f,-TraceDist), FQuat::Identity, CollisionChannel, CapsuleShape, QueryParams, ResponseParam);

		if (bBlockingHit)
		{
			// Reject hits adjacent to us, we only care about hits on the bottom portion of our capsule.
			// Check 2D distance to impact point, reject if within a tolerance from radius.
			if (Hit.bStartPenetrating || !IsWithinEdgeTolerance(CapsuleLocation, Hit.ImpactPoint, CapsuleShape.Capsule.Radius))
			{
				// Use a capsule with a slightly smaller radius and shorter height to avoid the adjacent object.
				ShrinkHeight = (PawnHalfHeight - PawnRadius) * (1.f - ShrinkScaleOverlap);
				TraceDist = SweepDistance + ShrinkHeight;
				CapsuleShape.Capsule.Radius = FMath::Max(0.f, CapsuleShape.Capsule.Radius - SWEEP_EDGE_REJECT_DISTANCE - KINDA_SMALL_NUMBER);
				CapsuleShape.Capsule.HalfHeight = FMath::Max(PawnHalfHeight - ShrinkHeight, 0.1f);
				bBlockingHit = GetWorld()->SweepSingle(Hit, CapsuleLocation, CapsuleLocation + FVector(0.f,0.f,-TraceDist), FQuat::Identity, CollisionChannel, CapsuleShape, QueryParams, ResponseParam);
			}

			// Reduce hit distance by ShrinkHeight because we shrank the capsule for the trace.
			// We allow negative distances here, because this allows us to pull out of penetrations.
			const float MaxPenetrationAdjust = FMath::Max(MAX_FLOOR_DIST, PawnRadius);
			const float SweepResult = FMath::Max(-MaxPenetrationAdjust, Hit.Time * TraceDist - ShrinkHeight);

			OutFloorResult.SetFromSweep(Hit, SweepResult, false);
			if (Hit.IsValidBlockingHit() && IsWalkable(Hit))
			{		
				if (SweepResult <= SweepDistance)
				{
					// Hit within test distance.
					OutFloorResult.bWalkableFloor = true;
					return;
				}
			}
		}
	}

	// Since we require a longer sweep than line trace, we don't want to run the line trace if the sweep missed everything.
	// We do however want to try a line trace if the sweep was stuck in penetration.
	if (!OutFloorResult.bBlockingHit && !OutFloorResult.HitResult.bStartPenetrating)
	{
		OutFloorResult.FloorDist = SweepDistance;
		return;
	}

	// Line trace
	if (LineDistance > 0.f)
	{
		const float ShrinkHeight = PawnHalfHeight;
		const FVector LineTraceStart = CapsuleLocation;	
		const float TraceDist = LineDistance + ShrinkHeight;
		const FVector Down = FVector(0.f, 0.f, -TraceDist);

		static const FName FloorLineTraceName = FName(TEXT("ComputeFloorDistLineTrace"));
		QueryParams.TraceTag = FloorLineTraceName;

		FHitResult Hit(1.f);
		bBlockingHit = GetWorld()->LineTraceSingle(Hit, LineTraceStart, LineTraceStart + Down, CollisionChannel, QueryParams, ResponseParam);

		if (bBlockingHit)
		{
			if (Hit.Time > 0.f)
			{
				// Reduce hit distance by ShrinkHeight because we started the trace higher than the base.
				// We allow negative distances here, because this allows us to pull out of penetrations.
				const float MaxPenetrationAdjust = FMath::Max(MAX_FLOOR_DIST, PawnRadius);
				const float LineResult = FMath::Max(-MaxPenetrationAdjust, Hit.Time * TraceDist - ShrinkHeight);

				OutFloorResult.bBlockingHit = true;
				if (LineResult <= LineDistance && IsWalkable(Hit))
				{
					OutFloorResult.SetFromLineTrace(Hit, OutFloorResult.FloorDist, LineResult, true);
					return;
				}
			}
		}
	}
	
	// No hits were acceptable.
	OutFloorResult.bWalkableFloor = false;
	OutFloorResult.FloorDist = SweepDistance;
}


void UCharacterMovementComponent::FindFloor(const FVector& CapsuleLocation, FFindFloorResult& OutFloorResult, bool bZeroDelta, const FHitResult* DownwardSweepResult) const
{
	// No collision, no floor...
	if (!UpdatedComponent->IsCollisionEnabled())
	{
		OutFloorResult.Clear();
		return;
	}

	// Increase height check slightly if walking, to prevent floor height adjustment from later invalidating the floor result.
	const float HeightCheckAdjust = (IsMovingOnGround() ? MAX_FLOOR_DIST + KINDA_SMALL_NUMBER : -MAX_FLOOR_DIST);

	float FloorSweepTraceDist = FMath::Max(MAX_FLOOR_DIST, MaxStepHeight + HeightCheckAdjust);
	float FloorLineTraceDist = FloorSweepTraceDist;
	bool bNeedToValidateFloor = true;
	
	// Sweep floor
	if (FloorLineTraceDist > 0.f || FloorSweepTraceDist > 0.f)
	{
		UCharacterMovementComponent* MutableThis = const_cast<UCharacterMovementComponent*>(this);

		if ( bAlwaysCheckFloor || !bZeroDelta || bForceNextFloorCheck || bJustTeleported )
		{
			MutableThis->bForceNextFloorCheck = false;
			ComputeFloorDist(CapsuleLocation, FloorLineTraceDist, FloorSweepTraceDist, OutFloorResult, CharacterOwner->CapsuleComponent->GetScaledCapsuleRadius(), DownwardSweepResult);
		}
		else
		{
			// Force floor check if base has collision disabled or if it does not block us.
			UPrimitiveComponent* MovementBase = CharacterOwner->GetMovementBase();
			const AActor* BaseActor = MovementBase ? MovementBase->GetOwner() : NULL;
			const ECollisionChannel CollisionChannel = UpdatedComponent->GetCollisionObjectType();

			if (!bForceNextFloorCheck && MovementBase != NULL)
			{
				MutableThis->bForceNextFloorCheck = !MovementBase->IsCollisionEnabled()
				|| MovementBase->GetCollisionResponseToChannel(CollisionChannel) != ECR_Block
				|| (MovementBase->Mobility == EComponentMobility::Movable)
				|| (Cast<const ADestructibleActor>(BaseActor) != NULL);
			}

			const bool IsActorBasePendingKill = BaseActor && BaseActor->IsPendingKill();

			if ( !bForceNextFloorCheck && !IsActorBasePendingKill && MovementBase &&
				(MovementBase->IsWorldGeometry() || (CharacterOwner->RelativeMovement.Location == CharacterOwner->GetActorLocation() - MovementBase->GetComponentLocation()))
				)
			{
				//UE_LOG(LogCharacterMovement, Log, TEXT("%s SKIP check for floor"), *CharacterOwner->GetName());
				OutFloorResult = CurrentFloor;
				bNeedToValidateFloor = false;
			}
			else
			{
				MutableThis->bForceNextFloorCheck = false;
				ComputeFloorDist(CapsuleLocation, FloorLineTraceDist, FloorSweepTraceDist, OutFloorResult, CharacterOwner->CapsuleComponent->GetScaledCapsuleRadius(), DownwardSweepResult);
			}
		}
	}

	// OutFloorResult.HitResult is now the result of the vertical floor check.
	// See if we should try to "perch" at this location.
	if (bNeedToValidateFloor && OutFloorResult.bBlockingHit && !OutFloorResult.bLineTrace)
	{
		const bool bCheckRadius = true;
		if (ShouldComputePerchResult(OutFloorResult.HitResult, bCheckRadius))
		{
			float MaxPerchFloorDist = FMath::Max(MAX_FLOOR_DIST, MaxStepHeight + HeightCheckAdjust);
			if (IsMovingOnGround())
			{
				MaxPerchFloorDist += FMath::Max(0.f, PerchAdditionalHeight);
			}

			FFindFloorResult PerchFloorResult;
			if (ComputePerchResult(GetValidPerchRadius(), OutFloorResult.HitResult, MaxPerchFloorDist, PerchFloorResult))
			{
				// Don't allow the floor distance adjustment to push us up too high, or we will move beyond the perch distance and fall next time.
				const float AvgFloorDist = (MIN_FLOOR_DIST + MAX_FLOOR_DIST) * 0.5f;
				const float MoveUpDist = (AvgFloorDist - OutFloorResult.FloorDist);
				if (MoveUpDist + PerchFloorResult.FloorDist >= MaxPerchFloorDist)
				{
					OutFloorResult.FloorDist = AvgFloorDist;
				}

				// If the regular capsule is on an unwalkable surface but the perched one would allow us to stand, override the normal to be one that is walkable.
				if (!OutFloorResult.bWalkableFloor)
				{
					OutFloorResult.SetFromLineTrace(PerchFloorResult.HitResult, OutFloorResult.FloorDist, FMath::Min(PerchFloorResult.FloorDist, PerchFloorResult.LineDist), true);
				}
			}
			else
			{
				// We had no floor (or an invalid one because it was unwalkable), and couldn't perch here, so invalidate floor (which will cause us to start falling).
				OutFloorResult.bWalkableFloor = false;
			}
		}
	}
}


bool UCharacterMovementComponent::IsValidLandingSpot(const FVector& CapsuleLocation, const FHitResult& Hit) const
{
	if (!Hit.bBlockingHit)
	{
		return false;
	}

	// Reject unwalkable floor normals.
	if (!IsWalkable(Hit))
	{
		return false;
	}

	// This can happen when landing on upward moving geometry
	if (Hit.bStartPenetrating)
	{
		return true;
	}

	float PawnRadius, PawnHalfHeight;
	CharacterOwner->CapsuleComponent->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);

	// Reject hits that are above our lower hemisphere (can happen when sliding down a vertical surface).
	const float LowerHemisphereZ = Hit.Location.Z - PawnHalfHeight + PawnRadius;
	if (Hit.ImpactPoint.Z >= LowerHemisphereZ)
	{
		return false;
	}

	// Reject hits that are barely on the cusp of the radius of the capsule
	if (!IsWithinEdgeTolerance(Hit.Location, Hit.ImpactPoint, PawnRadius))
	{
		return false;
	}

	FFindFloorResult FloorResult;
	FindFloor(CapsuleLocation, FloorResult, false, &Hit);

	if (!FloorResult.IsWalkableFloor())
	{
		return false;
	}

	return true;
}


float UCharacterMovementComponent::GetPerchRadiusThreshold() const
{
	// Don't allow negative values.
	return FMath::Max(0.f, PerchRadiusThreshold);
}


float UCharacterMovementComponent::GetValidPerchRadius() const
{
	if (CharacterOwner)
	{
		const float PawnRadius = CharacterOwner->CapsuleComponent->GetScaledCapsuleRadius();
		return FMath::Clamp(PawnRadius - GetPerchRadiusThreshold(), 0.1f, PawnRadius);
	}
	return 0.f;
}


bool UCharacterMovementComponent::ShouldComputePerchResult(const FHitResult& InHit, bool bCheckRadius) const
{
	if (!InHit.IsValidBlockingHit())
	{
		return false;
	}

	// Don't try to perch if the edge radius is very small.
	if (GetPerchRadiusThreshold() <= SWEEP_EDGE_REJECT_DISTANCE)
	{
		return false;
	}

 	if (bCheckRadius)
	{
		const float DistFromCenterSq = (InHit.ImpactPoint - InHit.Location).SizeSquared2D();
		const float StandOnEdgeRadius = GetValidPerchRadius();
		if (DistFromCenterSq <= FMath::Square(StandOnEdgeRadius))
		{
			// Already within perch radius.
			return false;
		}
	}
	
	return true;
}


bool UCharacterMovementComponent::ComputePerchResult(const float TestRadius, const FHitResult& InHit, const float InMaxFloorDist, FFindFloorResult& OutPerchFloorResult) const
{
	if (InMaxFloorDist <= 0.f)
	{
		return 0.f;
	}

	// Sweep further than actual requested distance, because a reduced capsule radius means we could miss some hits that the normal radius would contact.
	float PawnRadius, PawnHalfHeight;
	CharacterOwner->CapsuleComponent->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);

	const float InHitAboveBase = FMath::Max(0.f, InHit.ImpactPoint.Z - (InHit.Location.Z - PawnHalfHeight));
	const float PerchLineDist = FMath::Max(0.f, InMaxFloorDist - InHitAboveBase);
	const float PerchSweepDist = FMath::Max(0.f, InMaxFloorDist);

	const float ActualSweepDist = PerchSweepDist + PawnRadius;
	ComputeFloorDist(InHit.Location, PerchLineDist, ActualSweepDist, OutPerchFloorResult, TestRadius);

	if (!OutPerchFloorResult.IsWalkableFloor())
	{
		return false;
	}
	else if (InHitAboveBase + OutPerchFloorResult.FloorDist > InMaxFloorDist)
	{
		// Hit something past max distance
		OutPerchFloorResult.bWalkableFloor = false;
		return false;
	}

	return true;
}


bool UCharacterMovementComponent::CanStepUp(const FHitResult& Hit) const
{
	if (!Hit.IsValidBlockingHit() || !CharacterOwner || MovementMode == MOVE_Falling)
	{
		return false;
	}

	// No component for "fake" hits when we are on a known good base.
	const UPrimitiveComponent* HitComponent = Hit.Component.Get();
	if (!HitComponent)
	{
		return true;
	}

	if (!HitComponent->CanBeBaseForCharacter(CharacterOwner))
	{
		return false;
	}

	// No actor for "fake" hits when we are on a known good base.
	const AActor* HitActor = Hit.GetActor();
	if (!HitActor)
	{
		 return true;
	}

	if (!HitActor->CanBeBaseForCharacter(CharacterOwner))
	{
		return false;
	}

	return true;
}


bool UCharacterMovementComponent::StepUp(const FVector& GravDir, const FVector& Delta, const FHitResult &InHit, FStepDownResult* OutStepDownResult)
{
	if (!CanStepUp(InHit))
	{
		return false;
	}

	if (MaxStepHeight <= 0.f)
	{
		return false;
	}

	const FVector OldLocation = UpdatedComponent->GetComponentLocation();
	float PawnRadius, PawnHalfHeight;
	CharacterOwner->CapsuleComponent->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);

	// Don't bother stepping up if top of capsule is hitting something.
	const float InitialImpactZ = InHit.ImpactPoint.Z;
	if (InitialImpactZ > OldLocation.Z + (PawnHalfHeight - PawnRadius))
	{
		return false;
	}

	// Don't step up if the impact is below us
	if (InitialImpactZ <= OldLocation.Z - PawnHalfHeight)
	{
		return false;
	}

	float StepTravelHeight = MaxStepHeight;
	const float StepSideZ = -1.f * (InHit.ImpactNormal | GravDir);
	float PawnInitialFloorBaseZ = OldLocation.Z - PawnHalfHeight;
	float PawnFloorPointZ = PawnInitialFloorBaseZ;

	if (IsMovingOnGround() && CurrentFloor.IsWalkableFloor())
	{
		// Since we float a variable amount off the floor, we need to enforce max step height off the actual point of impact with the floor.
		const float FloorDist = FMath::Max(0.f, CurrentFloor.FloorDist);
		PawnInitialFloorBaseZ -= FloorDist;
		StepTravelHeight = FMath::Max(StepTravelHeight - FloorDist, 0.f);

		const bool bHitVerticalFace = !IsWithinEdgeTolerance(InHit.Location, InHit.ImpactPoint, PawnRadius);
		if (!CurrentFloor.bLineTrace && !bHitVerticalFace)
		{
			PawnFloorPointZ = CurrentFloor.HitResult.ImpactPoint.Z;
		}
		else
		{
			// Base floor point is the base of the capsule moved down by how far we are hovering over the surface we are hitting.
			PawnFloorPointZ -= CurrentFloor.FloorDist;
		}
	}

	// Scope our movement updates, and do not apply them until all intermediate moves are completed.
	FScopedMovementUpdate ScopedStepUpMovement(UpdatedComponent, EScopedUpdate::DeferredUpdates);

	// step up - treat as vertical wall
	FHitResult SweepUpHit(1.f);
	const FRotator PawnRotation = CharacterOwner->GetActorRotation();
	SafeMoveUpdatedComponent( -GravDir * StepTravelHeight, PawnRotation, true, SweepUpHit);

	// step fwd
	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent( Delta, PawnRotation, true, Hit);

	// If we hit something above us and also something ahead of us, we should notify about the upward hit as well.
	// The forward hit will be handled later (in the bSteppedOver case below).
	// In the case of hitting something above but not forward, we are not blocked from moving so we don't need the notification.
	if (SweepUpHit.bBlockingHit && Hit.bBlockingHit)
	{
		HandleImpact(SweepUpHit);
	}

	// Check result of forward movement
	if (Hit.bBlockingHit)
	{
		if (Hit.bStartPenetrating)
		{
			// Undo movement
			ScopedStepUpMovement.RevertMove();
			return false;
		}

		// pawn ran into a wall
		HandleImpact(Hit);
		if ( IsFalling() )
		{
			return true;
		}

		// adjust and try again
		SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);
		if ( IsFalling() )
		{
			ScopedStepUpMovement.RevertMove();
			return false;
		}
	}
	
	// Step down
	SafeMoveUpdatedComponent(GravDir * (MaxStepHeight + MAX_FLOOR_DIST*2.f), CharacterOwner->GetActorRotation(), true, Hit);

	// If step down was initially penetrating abort the step up
	if (Hit.bStartPenetrating)
	{
		ScopedStepUpMovement.RevertMove();
		return false;
	}

	FStepDownResult StepDownResult;
	if (Hit.IsValidBlockingHit())
	{
		const FVector HitLocation = Hit.Location;

		// See if the downward move impacts on the lower capsule hemisphere
		const float CurBaseLocation = (HitLocation.Z - PawnHalfHeight);
		const float LowerImpactHeight = Hit.ImpactPoint.Z - CurBaseLocation;
		if (LowerImpactHeight <= PawnRadius)
		{
			// See if this step sequence would have allowed us to travel higher than our max step height allows.
			const float DeltaZ = Hit.ImpactPoint.Z - PawnFloorPointZ;
			if (DeltaZ > MaxStepHeight)
			{
				//UE_LOG(LogCharacterMovement, VeryVerbose, TEXT("- Reject StepUp (too high Height %.3f) up from floor base %f to %f"), DeltaZ, PawnInitialFloorBaseZ, NewLocation.Z);
				ScopedStepUpMovement.RevertMove();
				return false;
			}
		}

		// Reject unwalkable surface normals here.
		if (!IsWalkable(Hit))
		{
			// Reject if normal is towards movement direction
			const bool bNormalTowardsMe = (Delta | Hit.ImpactNormal) < 0.f;
			if (bNormalTowardsMe)
			{
				//UE_LOG(LogCharacterMovement, VeryVerbose, TEXT("- Reject StepUp (unwalkable normal %s opposed to movement)"), *Hit.ImpactNormal.ToString());
				ScopedStepUpMovement.RevertMove();
				return false;
			}

			// Also reject if we would end up being higher than our starting location by stepping down.
			// It's fine to step down onto an unwalkable normal below us, we will just slide off. Rejecting those moves would prevent us from being able to walk off the edge.
			if (HitLocation.Z > OldLocation.Z)
			{
				//UE_LOG(LogCharacterMovement, VeryVerbose, TEXT("- Reject StepUp (unwalkable normal %s above old position)"), *Hit.ImpactNormal.ToString());
				ScopedStepUpMovement.RevertMove();
				return false;
			}
		}

		// See if we can validate the floor as a result of this step down. In almost all cases this should succeed, and we can avoid computing the floor outside this method.
		if (OutStepDownResult != NULL)
		{
			FindFloor(UpdatedComponent->GetComponentLocation(), StepDownResult.FloorResult, false, &Hit);

			// Reject unwalkable normals if we end up higher than our initial height.
			// It's fine to walk down onto an unwalkable surface, don't reject those moves.
			if (HitLocation.Z > OldLocation.Z)
			{
				// We should reject the floor result if we are trying to step up an actual step where we are not able to perch (this is rare).
				// In those cases we should instead abort the step up and try to slide along the stair.
				if (!StepDownResult.FloorResult.bBlockingHit && StepSideZ < MAX_STEP_SIDE_Z)
				{
					ScopedStepUpMovement.RevertMove();
					return false;
				}
			}

			StepDownResult.bComputedFloor = true;
		}
	}
	
	// Copy step down result.
	if (OutStepDownResult != NULL)
	{
		*OutStepDownResult = StepDownResult;
	}

	// Don't recalculate velocity based on this height adjustment, if considering vertical adjustments.
	bJustTeleported |= !bMaintainHorizontalGroundVelocity;

	return true;
}

void UCharacterMovementComponent::HandleImpact(FHitResult const& Impact, float TimeSlice, const FVector& MoveDelta)
{
	if (CharacterOwner)
	{
		CharacterOwner->MoveBlockedBy(Impact);
	}

	if (PathFollowingComp.IsValid())
	{	// Also notify path following!
		PathFollowingComp->OnMoveBlockedBy(Impact);
	}

	APawn* OtherPawn = Cast<APawn>(Impact.GetActor());
	if (OtherPawn)
	{
		NotifyBumpedPawn(OtherPawn);
	}

	if (bEnablePhysicsInteraction && Impact.Component != NULL && Impact.Component->IsAnySimulatingPhysics() && Impact.bBlockingHit)
	{
		FVector ForcePoint = Impact.ImpactPoint;

		FBodyInstance* BI = Impact.Component->GetBodyInstance(Impact.BoneName);
		
		float BodyMass = 1.0f;
		
		if (BI != NULL)
		{
			BodyMass = FMath::Max(BI->GetBodyMass(), 1.0f);

			FBox Bounds = BI->GetBodyBounds();

			FVector Center, Extents;
			Bounds.GetCenterAndExtents(Center, Extents);

			if (!Extents.IsNearlyZero())
			{
				ForcePoint.Z = Center.Z + Extents.Z * PushForcePointZOffsetFactor;
			}
		}
		
		FVector Force = Impact.ImpactNormal * -1.0f;
		Force.Normalize();
			
		float PushForceModificator = 1.0f;

		FVector CurrentVelocity = Impact.Component->GetPhysicsLinearVelocity();
		FVector VirtualVelocity = Acceleration.SafeNormal() * GetMaxSpeed();

		float Dot = 0.0f;
		
		if (bScalePushForceToVelocity && !CurrentVelocity.IsNearlyZero())
		{			
			Dot = CurrentVelocity | VirtualVelocity;
			
			if (Dot > 0.0f && Dot < 1.0f)
			{
				PushForceModificator *= Dot;
			}
		}

		if (bPushForceScaledToMass)
		{
			PushForceModificator *= BodyMass;
		}

		Force *= PushForceModificator;

		if (CurrentVelocity.IsNearlyZero())
		{
			Force *= InitialPushForceFactor;
			Impact.Component->AddImpulseAtLocation(Force, ForcePoint, Impact.BoneName);
		}
		else
		{
			Force *= PushForceFactor;
			Impact.Component->AddForceAtLocation(Force, ForcePoint, Impact.BoneName);
		}
	}
}

FString UCharacterMovementComponent::GetMovementName()
{
	if( CharacterOwner )
	{
		if ( CharacterOwner->GetRootComponent() && CharacterOwner->GetRootComponent()->IsSimulatingPhysics() )
		{
			return TEXT("Rigid Body");
		}
		else if ( CharacterOwner->IsMatineeControlled() )
		{
			return TEXT("Matinee");
		}
	}

	// Using character movement
	switch( MovementMode )
	{
		case MOVE_None:				return TEXT("NULL"); break;
		case MOVE_Walking:			return TEXT("Walking"); break;
		case MOVE_Falling:			return TEXT("Falling"); break;
		case MOVE_Swimming:			return TEXT("Swimming"); break;
		case MOVE_Flying:			return TEXT("Flying"); break;
		case MOVE_Custom:			return TEXT("Custom"); break;
		case MOVE_Unused:			return TEXT("Unused"); break;
	}
	return TEXT("Unknown");
}

void UCharacterMovementComponent::DisplayDebug(UCanvas* Canvas, const TArray<FName>& DebugDisplay, float& YL, float& YPos)
{
	if ( CharacterOwner == NULL )
	{
		return;
	}

	Canvas->SetDrawColor(255,255,255);
	UFont* RenderFont = GEngine->GetSmallFont();
	FString T = FString::Printf(TEXT("CHARACTER MOVEMENT Floor %s Crouched %i"), *CurrentFloor.HitResult.ImpactNormal.ToString(), IsCrouching());
	Canvas->DrawText(RenderFont, T, 4.0f, YPos );
	YPos += YL;

	T = FString::Printf(TEXT("Updated Component: %s"), *UpdatedComponent->GetName());
	Canvas->DrawText(RenderFont, T, 4.0f, YPos );
	YPos += YL;

	T = FString::Printf(TEXT("bForceMaxAccel: %i"), bForceMaxAccel);
	Canvas->DrawText(RenderFont, T, 4.0f, YPos );
	YPos += YL;

	APhysicsVolume * PhysicsVolume = GetPhysicsVolume();

	const UPrimitiveComponent* BaseComponent = CharacterOwner->GetMovementBase();
	const AActor* BaseActor = BaseComponent ? BaseComponent->GetOwner() : NULL;

	T = FString::Printf(TEXT("%s In physicsvolume %s on base %s component %s gravity %f"), *GetMovementName(), (PhysicsVolume ? *PhysicsVolume->GetName() : TEXT("None")),
		(BaseActor ? *BaseActor->GetName() : TEXT("None")), (BaseComponent ? *BaseComponent->GetName() : TEXT("None")), GetGravityZ());
	Canvas->DrawText(RenderFont, T, 4.0f, YPos );
	YPos += YL;
}


void UCharacterMovementComponent::ForceReplicationUpdate()
{
	if (HasPredictionData_Server())
	{
		GetPredictionData_Server_Character()->LastUpdateTime = GetWorld()->TimeSeconds - 10.f;
	}
}


FVector UCharacterMovementComponent::ConstrainInputAcceleration(const FVector& InputAcceleration) const
{
	FVector NewAccel = InputAcceleration;

	// walking or falling pawns ignore up/down sliding
	if (IsMovingOnGround() || IsFalling())
	{
		NewAccel.Z = 0.f;
	}

	return NewAccel;
}


FVector UCharacterMovementComponent::ScaleInputAcceleration(const FVector& InputAcceleration) const
{
	if (InputAcceleration.SizeSquared() > 0.f)
	{
		return GetModifiedMaxAcceleration() * SafeNormalPrecise(InputAcceleration);
	}

	return FVector::ZeroVector;
}



void UCharacterMovementComponent::SmoothCorrection(const FVector& OldLocation)
{
	if (!CharacterOwner || GetNetMode() != NM_Client)
	{
		return;
	}

	FNetworkPredictionData_Client_Character* ClientData = GetPredictionData_Client_Character();
	if (ClientData && ClientData->bSmoothNetUpdates)
	{
		float DistSq = (OldLocation - CharacterOwner->GetActorLocation()).SizeSquared();
		if (DistSq > FMath::Square(ClientData->MaxSmoothNetUpdateDist))
		{
			ClientData->MeshTranslationOffset = (DistSq > FMath::Square(ClientData->NoSmoothNetUpdateDist)) 
				? FVector::ZeroVector 
				: ClientData->MeshTranslationOffset + ClientData->MaxSmoothNetUpdateDist * (OldLocation - CharacterOwner->GetActorLocation()).SafeNormal();	
		}
		else
		{
			ClientData->MeshTranslationOffset = ClientData->MeshTranslationOffset + OldLocation - CharacterOwner->GetActorLocation();	
		}
	}
}


void UCharacterMovementComponent::SmoothClientPosition(float DeltaSeconds)
{
	if (!CharacterOwner || GetNetMode() != NM_Client)
	{
		return;
	}

	FNetworkPredictionData_Client_Character* ClientData = GetPredictionData_Client_Character();
	if (ClientData && ClientData->bSmoothNetUpdates)
	{
		// smooth interpolation of mesh translation to avoid popping of other client pawns, unless driving or ragdoll or low tick rate
		if ((DeltaSeconds < ClientData->SmoothNetUpdateTime) && CharacterOwner->Mesh && !CharacterOwner->Mesh->IsSimulatingPhysics())
		{
			ClientData->MeshTranslationOffset = (ClientData->MeshTranslationOffset * (1.f - DeltaSeconds / ClientData->SmoothNetUpdateTime));
		}
		else
		{
			ClientData->MeshTranslationOffset = FVector::ZeroVector;
		}

		if (IsMovingOnGround())
		{
			// don't smooth Z position if walking on ground
			ClientData->MeshTranslationOffset.Z = 0;
		}

		if (CharacterOwner->Mesh)
		{
			const FVector NewRelTranslation = CharacterOwner->ActorToWorld().InverseTransformVector(ClientData->MeshTranslationOffset + CharacterOwner->GetBaseTranslationOffset());
			CharacterOwner->Mesh->SetRelativeLocation(NewRelTranslation);
		}
	}
}


bool UCharacterMovementComponent::ClientUpdatePositionAfterServerUpdate()
{
	if (!IsValid(CharacterOwner))
	{
		return false;
	}

	FNetworkPredictionData_Client_Character* ClientData = GetPredictionData_Client_Character();
	check(ClientData);

	if (!ClientData->bUpdatePosition)
	{
		return false;
	}

	ClientData->bUpdatePosition = false;

	// Don't do any network position updates on things running PHYS_RigidBody
	if (CharacterOwner->GetRootComponent() && CharacterOwner->GetRootComponent()->IsSimulatingPhysics())
	{
		return false;
	}

	if (ClientData->SavedMoves.Num() == 0)
	{
		UE_LOG(LogNetPlayerMovement, VeryVerbose, TEXT("ClientUpdatePositionAfterServerUpdate No saved moves to replay"), ClientData->SavedMoves.Num());
		return false;
	}

	FRootMotionMovementParams BackupRootMotionParams = RootMotionParams;
	bool bRealJump = CharacterOwner->bPressedJump;
	bool bRealCrouch = bWantsToCrouch;
	bool bRealForceMaxAccel = bForceMaxAccel;
	CharacterOwner->bClientWasFalling = (MovementMode == MOVE_Falling);
	CharacterOwner->bClientUpdating = true;
	bForceNextFloorCheck = true;

	// Replay moves that have not yet been acked.
	UE_LOG(LogNetPlayerMovement, VeryVerbose, TEXT("ClientUpdatePositionAfterServerUpdate Replaying Moves (%d)"), ClientData->SavedMoves.Num());
	for (int32 i=0; i<ClientData->SavedMoves.Num(); i++)
	{
		const FSavedMovePtr& CurrentMove = ClientData->SavedMoves[i];
		if (ClientData->PendingMove == CurrentMove)
		{
			ClientData->PendingMove->SetInitialPosition(CharacterOwner);
		}

		CurrentMove->PrepMoveFor(CharacterOwner);
		MoveAutonomous(CurrentMove->TimeStamp, CurrentMove->DeltaTime, CurrentMove->GetCompressedFlags(), CurrentMove->Acceleration);
		CurrentMove->ResetMoveFor(CharacterOwner);
	}

	RootMotionParams = BackupRootMotionParams;
	CharacterOwner->bClientResimulateRootMotion = false;
	CharacterOwner->bClientUpdating = false;
	CharacterOwner->bPressedJump = bRealJump;
	bWantsToCrouch = bRealCrouch;
	bForceMaxAccel = bRealForceMaxAccel;
	bForceNextFloorCheck = true;
	
	return (ClientData->SavedMoves.Num() > 0);
}


void UCharacterMovementComponent::ForcePositionUpdate(float DeltaTime)
{
	if (!IsValid(CharacterOwner) || MovementMode == MOVE_None)
	{
		return;
	}

	check(CharacterOwner->Role == ROLE_Authority);
	check(CharacterOwner->GetRemoteRole() == ROLE_AutonomousProxy);

	if (!Velocity.IsZero())
	{
		PerformMovement(DeltaTime);
	}
}


FNetworkPredictionData_Client* UCharacterMovementComponent::GetPredictionData_Client() const
{
	// Should only be called on client in network games
	check(CharacterOwner != NULL);
	check(CharacterOwner->Role < ROLE_Authority);
	check(GetNetMode() == NM_Client);

	if (!ClientPredictionData)
	{
		UCharacterMovementComponent* MutableThis = const_cast<UCharacterMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Character();
	}

	return ClientPredictionData;
}

FNetworkPredictionData_Server* UCharacterMovementComponent::GetPredictionData_Server() const
{
	// Should only be called on server in network games
	check(CharacterOwner != NULL);
	check(CharacterOwner->Role == ROLE_Authority);
	check(GetNetMode() < NM_Client);

	if (!ServerPredictionData)
	{
		UCharacterMovementComponent* MutableThis = const_cast<UCharacterMovementComponent*>(this);
		MutableThis->ServerPredictionData = new FNetworkPredictionData_Server_Character();
	}

	return ServerPredictionData;
}


FNetworkPredictionData_Client_Character* UCharacterMovementComponent::GetPredictionData_Client_Character() const
{
	return static_cast<class FNetworkPredictionData_Client_Character*>(GetPredictionData_Client());
}


FNetworkPredictionData_Server_Character* UCharacterMovementComponent::GetPredictionData_Server_Character() const
{
	return static_cast<class FNetworkPredictionData_Server_Character*>(GetPredictionData_Server());
}


void UCharacterMovementComponent::ResetPredictionData_Client()
{
	if (ClientPredictionData)
	{
		delete ClientPredictionData;
		ClientPredictionData = NULL;
	}
}

void UCharacterMovementComponent::ResetPredictionData_Server()
{
	if (ServerPredictionData)
	{
		delete ServerPredictionData;
		ServerPredictionData = NULL;
	}	
}

float FNetworkPredictionData_Client_Character::UpdateTimeStampAndDeltaTime(float DeltaTime, class ACharacter & CharacterOwner, class UCharacterMovementComponent & CharacterMovementComponent)
{
	// Reset TimeStamp regularly to combat float accuracy decreasing over time.
	if( CurrentTimeStamp > CharacterMovementComponent.MinTimeBetweenTimeStampResets )
	{
		UE_LOG(LogNetPlayerMovement, Log, TEXT("Resetting Client's TimeStamp %f"), CurrentTimeStamp);
		CurrentTimeStamp = 0.f;

		// Mark all buffered moves as having old time stamps, so we make sure to not resend them.
		// That would confuse the server.
		for(int32 MoveIndex=0; MoveIndex<SavedMoves.Num(); MoveIndex++)
		{
			const FSavedMovePtr& CurrentMove = SavedMoves[MoveIndex];
			SavedMoves[MoveIndex]->bOldTimeStampBeforeReset = true;
		}
		// Do LastAckedMove as well. No need to do PendingMove as that move is part of the SavedMoves array.
		if( LastAckedMove.IsValid() )
		{
			LastAckedMove->bOldTimeStampBeforeReset = true;
		}
	}

	// Update Current TimeStamp.
	CurrentTimeStamp += DeltaTime;
	float ClientDeltaTime = DeltaTime;

	// Server uses TimeStamps to derive DeltaTime which introduces some rounding errors.
	// Make sure we do the same, so MoveAutonomous uses the same inputs and is deterministic!!
	if( SavedMoves.Num() > 0 )
	{
		const FSavedMovePtr& PreviousMove = SavedMoves.Last();
		if( !PreviousMove->bOldTimeStampBeforeReset )
		{
			// How server will calculate its deltatime to update physics.
			const float ServerDeltaTime = CurrentTimeStamp - PreviousMove->TimeStamp;
			// Have client always use the Server's DeltaTime. Otherwise our physics simulation will differ and we'll trigger too many position corrections and increase our network traffic.
			ClientDeltaTime = ServerDeltaTime;
		}
	}

	return CharacterOwner.CustomTimeDilation * FMath::Min(ClientDeltaTime, MaxResponseTime * CharacterOwner.GetWorldSettings()->GetEffectiveTimeDilation());
}

void UCharacterMovementComponent::ReplicateMoveToServer(float DeltaTime, const FVector& NewAcceleration)
{
	check(CharacterOwner != NULL);

	// Can only start sending moves if our controllers are synced up over the network, otherwise we flood the reliable buffer.
	APlayerController* PC = Cast<APlayerController>(CharacterOwner->GetController());
	if (PC && PC->AcknowledgedPawn != CharacterOwner)
	{
		return;
	}

	FNetworkPredictionData_Client_Character* ClientData = GetPredictionData_Client_Character();
	if (!ClientData)
	{
		return;
	}
	
	// Update our delta time for physics simulation.
	DeltaTime = ClientData->UpdateTimeStampAndDeltaTime(DeltaTime, *CharacterOwner, *this);

	// Find the oldest (unacknowledged) important move (OldMove).
	// Don't include the last move because it may be combined with the next new move.
	// A saved move is interesting if it differs significantly from the last acknowledged move
	FSavedMovePtr OldMove = NULL;
	if( ClientData->LastAckedMove.IsValid() )
	{
		for (int32 i=0; i < ClientData->SavedMoves.Num()-1; i++)
		{
			const FSavedMovePtr& CurrentMove = ClientData->SavedMoves[i];
			if (CurrentMove->IsImportantMove(ClientData->LastAckedMove))
			{
				OldMove = CurrentMove;
				break;
			}
		}
	}

	// Get a SavedMove object to store the movement in.
	FSavedMovePtr NewMove = ClientData->CreateSavedMove();
	if (NewMove.IsValid() == false)
	{
		return;
	}

	NewMove->SetMoveFor(CharacterOwner, DeltaTime, NewAcceleration, *ClientData);

	// see if the two moves could be combined
	// do not combine moves which have different TimeStamps (before and after reset).
	if( ClientData->PendingMove.IsValid() && !ClientData->PendingMove->bOldTimeStampBeforeReset && ClientData->PendingMove->CanCombineWith(NewMove, CharacterOwner, ClientData->MaxResponseTime * CharacterOwner->GetWorldSettings()->GetEffectiveTimeDilation()))
	{
		// Only combine and move back to the start location if we don't move back in to a spot that would make us collide with something new.
		if (!OverlapTest(ClientData->PendingMove->GetStartLocation(), ClientData->PendingMove->StartRotation.Quaternion(), UpdatedComponent->GetCollisionObjectType(), GetPawnCapsuleCollisionShape(SHRINK_None), CharacterOwner))
		{
			FScopedMovementUpdate ScopedMovementUpdate(UpdatedComponent, EScopedUpdate::DeferredUpdates);
			UE_LOG(LogNetPlayerMovement, VeryVerbose, TEXT("CombineMove: add delta %f + %f and revert from %f %f to %f %f"), DeltaTime, ClientData->PendingMove->DeltaTime, CharacterOwner->GetActorLocation().X, CharacterOwner->GetActorLocation().Y, ClientData->PendingMove->GetStartLocation().X, ClientData->PendingMove->GetStartLocation().Y);
			
			// to combine move, first revert pawn position to PendingMove start position, before playing combined move on client
			const bool bNoCollisionCheck = true;
			UpdatedComponent->SetWorldLocationAndRotation(ClientData->PendingMove->GetStartLocation(), ClientData->PendingMove->StartRotation, false);
			Velocity = ClientData->PendingMove->StartVelocity;

			if (ClientData->PendingMove->StartBase != CharacterOwner->GetMovementBase())
			{
				CharacterOwner->SetBase( ClientData->PendingMove->StartBase.Get() );
			}
			CurrentFloor = ClientData->PendingMove->StartFloor;

			// Now that we have reverted to the old position, prepare a new move from that position,
			// using our current velocity, acceleration, and rotation, but applied over the combined time from the old and new move.

			NewMove->DeltaTime += ClientData->PendingMove->DeltaTime;
			NewMove->SetInitialPosition(CharacterOwner);

			// Remove pending move from move list. It would have to be the last move on the list.
			if (ClientData->SavedMoves.Num() > 0 && ClientData->SavedMoves.Last() == ClientData->PendingMove)
			{
				ClientData->SavedMoves.Pop();
			}
			ClientData->FreeMove(ClientData->PendingMove);
			ClientData->PendingMove = NULL;
		}
		else
		{
			//UE_LOG(LogNet, Log, TEXT("Not combining move, would collide at start location"));
		}
	}

	// Perform the move locally
	Acceleration = NewMove->Acceleration;
	CharacterOwner->ClientRootMotionParams.Clear();
	PerformMovement(NewMove->DeltaTime);

	NewMove->PostUpdate(CharacterOwner);

	// Add NewMove to the list
	ClientData->SavedMoves.Push(NewMove);

	static const auto CVarCombine = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("p.NetEnableMoveCombining"));
	const bool bCanCombine = (!CVarCombine || CVarCombine->GetValueOnGameThread() != 0);

	if (bCanCombine && ClientData->PendingMove.IsValid() == false)
	{
		// Decide whether to hold off on move
		// send moves more frequently in small games where server isn't likely to be saturated
		float NetMoveDelta;
		UPlayer* Player = (PC ? PC->Player : NULL);

		if (Player && (Player->CurrentNetSpeed > 10000) && (GetWorld()->GameState != NULL) && (GetWorld()->GameState->PlayerArray.Num() <= 10))
		{
			NetMoveDelta = 0.011f;
		}
		else if (Player && CharacterOwner->GetWorldSettings()->GameNetworkManagerClass) 
		{
			NetMoveDelta = FMath::Max(0.0222f,2 * GetDefault<AGameNetworkManager>(CharacterOwner->GetWorldSettings()->GameNetworkManagerClass)->MoveRepSize/Player->CurrentNetSpeed);
		}
		else
		{
			NetMoveDelta = 0.011f;
		}

		if ((GetWorld()->TimeSeconds - ClientData->ClientUpdateTime) * CharacterOwner->GetWorldSettings()->GetEffectiveTimeDilation() < NetMoveDelta)
		{
			ClientData->PendingMove = NewMove;
			return;
		}
	}

	ClientData->ClientUpdateTime = GetWorld()->TimeSeconds;

	UE_LOG(LogNetPlayerMovement, Verbose, TEXT("Client ReplicateMove Time %f Acceleration %s Position %s DeltaTime %f"),
		NewMove->TimeStamp, *NewMove->Acceleration.ToString(), *CharacterOwner->GetActorLocation().ToString(), DeltaTime);

	// Send to the server
	// Compress rotation down to 5 bytes
	const FRotator ControllerRot = CharacterOwner->GetControlRotation().GetNormalized();
	const uint32 ClientYawShort = FRotator::CompressAxisToShort(ControllerRot.Yaw);
	const uint32 ClientPitchShort = FRotator::CompressAxisToShort(ControllerRot.Pitch);
	const uint32 RotationINT = (ClientYawShort << 16) | ClientPitchShort;
	const uint8 ClientRollBYTE = FRotator::CompressAxisToByte(ControllerRot.Roll);

	CallServerMove( NewMove.Get(),
		CharacterOwner->GetActorLocation(),
		ClientRollBYTE,
		RotationINT,
		OldMove.Get() );

	ClientData->PendingMove = NULL;
}

void UCharacterMovementComponent::CallServerMove
	(
	class FSavedMove_Character* NewMove,
	const FVector& ClientLoc,
	uint8 ClientRoll,
	int32 View,
	class FSavedMove_Character* OldMove
	)
{
	// compress old move if it exists
	if (OldMove)
	{
		CharacterOwner->ServerMoveOld(OldMove->TimeStamp, OldMove->Acceleration, OldMove->GetCompressedFlags());
	}

	FNetworkPredictionData_Client_Character* ClientData = GetPredictionData_Client_Character();
	if (ClientData->PendingMove.IsValid())
	{
		//@hack
		int32 RotationINT = (((int32)(182.0444f * ClientData->PendingMove->ControlRotation.Yaw) & 65335) << 16) + ((int32)(182.0444f * ClientData->PendingMove->ControlRotation.Pitch) & 65335);

		// send two moves simultaneously
		CharacterOwner->ServerMoveDual
			(
			ClientData->PendingMove->TimeStamp,
			ClientData->PendingMove->Acceleration,
			ClientData->PendingMove->GetCompressedFlags(),
			RotationINT,
			NewMove->TimeStamp,
			NewMove->Acceleration,
			ClientLoc,
			NewMove->GetCompressedFlags(),
			ClientRoll,
			View
			);
	}
	else
	{
		CharacterOwner->ServerMove
			(
			NewMove->TimeStamp,
			NewMove->Acceleration,
			ClientLoc,
			NewMove->GetCompressedFlags(),
			ClientRoll,
			View
			);
	}


	APlayerController* PC = Cast<APlayerController>(CharacterOwner->GetController());
	APlayerCameraManager* PlayerCameraManager = (PC ? PC->PlayerCameraManager : NULL);
	if (PlayerCameraManager != NULL && PlayerCameraManager->bUseClientSideCameraUpdates)
	{
		PlayerCameraManager->bShouldSendClientSideCameraUpdate = true;
	}
}



void UCharacterMovementComponent::ServerMoveOld_Implementation
	(
	float OldTimeStamp,
	FVector_NetQuantize100 OldAccel,
	uint8 OldMoveFlags
	)
{
	if (!IsValid(CharacterOwner) || !IsComponentTickEnabled())
	{
		return;
	}

	FNetworkPredictionData_Server_Character* ServerData = GetPredictionData_Server_Character();
	check(ServerData);

	if( !VerifyClientTimeStamp(OldTimeStamp, *ServerData) )
	{
		return;
	}

	UE_LOG(LogNetPlayerMovement, Log, TEXT("Recovered move from OldTimeStamp %f, DeltaTime: %f"), OldTimeStamp, OldTimeStamp - ServerData->CurrentClientTimeStamp);
	const float MaxResponseTime = ServerData->MaxResponseTime * CharacterOwner->GetWorldSettings()->GetEffectiveTimeDilation();

	MoveAutonomous(OldTimeStamp, FMath::Min(OldTimeStamp - ServerData->CurrentClientTimeStamp, MaxResponseTime), OldMoveFlags, OldAccel);

	ServerData->CurrentClientTimeStamp = OldTimeStamp;
}


void UCharacterMovementComponent::ServerMoveDual_Implementation(float TimeStamp0, FVector_NetQuantize100 InAccel0, uint8 PendingFlags, uint32 View0, float TimeStamp, FVector_NetQuantize100 InAccel, FVector_NetQuantize100 ClientLoc, uint8 NewFlags, uint8 ClientRoll, uint32 View)
{
	ServerMove_Implementation(TimeStamp0, InAccel0, FVector(1.f,2.f,3.f), PendingFlags, ClientRoll, View0);
	ServerMove_Implementation(TimeStamp, InAccel, ClientLoc, NewFlags, ClientRoll, View);
}

bool UCharacterMovementComponent::VerifyClientTimeStamp(float TimeStamp, FNetworkPredictionData_Server_Character & ServerData)
{
	// Very large deltas happen around a TimeStamp reset.
	const float DeltaTimeStamp = (TimeStamp - ServerData.CurrentClientTimeStamp);
	if( FMath::Abs(DeltaTimeStamp) > (MinTimeBetweenTimeStampResets * 0.5f) )
	{
		// Client is resetting TimeStamp to increase accuracy.
		if( DeltaTimeStamp < 0.f )
		{
			UE_LOG(LogNetPlayerMovement, Log, TEXT("TimeStamp reset detected. CurrentTimeStamp: %f, new TimeStamp: %f"), ServerData.CurrentClientTimeStamp, TimeStamp);
			ServerData.CurrentClientTimeStamp = 0.f;
			return true;
		}
		else
		{
			// We already reset the TimeStamp, but we just got an old outdated move before the switch.
			// Just ignore it.
			UE_LOG(LogNetPlayerMovement, Log, TEXT("TimeStamp expired. Before TimeStamp Reset. CurrentTimeStamp: %f, TimeStamp: %f"), ServerData.CurrentClientTimeStamp, TimeStamp);
			return false;
		}
	}

	// If TimeStamp is in the past, move is outdated, ignore it.
	if( TimeStamp <= ServerData.CurrentClientTimeStamp )
	{
		UE_LOG(LogNetPlayerMovement, Log, TEXT("TimeStamp expired. %f, CurrentTimeStamp: %f"), TimeStamp, ServerData.CurrentClientTimeStamp);
		return false;
	}
	
	UE_LOG(LogNetPlayerMovement, VeryVerbose, TEXT("TimeStamp %f Accepted! CurrentTimeStamp: %f"), TimeStamp, ServerData.CurrentClientTimeStamp);
	return true;
}

void UCharacterMovementComponent::ServerMove_Implementation(float TimeStamp, FVector_NetQuantize100 InAccel, FVector_NetQuantize100 ClientLoc, uint8 MoveFlags, uint8 ClientRoll, uint32 View)
{
	if (!IsValid(CharacterOwner) || !IsComponentTickEnabled())
	{
		return;
	}	

	FNetworkPredictionData_Server_Character* ServerData = GetPredictionData_Server_Character();
	check(ServerData);

	if( !VerifyClientTimeStamp(TimeStamp, *ServerData) )
	{
		return;
	}

	bool bServerReadyForClient = true;
	APlayerController* PC = Cast<APlayerController>(CharacterOwner->GetController());
	if (PC)
	{
		bServerReadyForClient = PC->NotifyServerReceivedClientData(CharacterOwner, TimeStamp);
		if (!bServerReadyForClient)
		{
			InAccel = FVector::ZeroVector;
		}
	}

	// View components
	const uint16 ViewPitch = (View & 65535);
	const uint16 ViewYaw = (View >> 16);
	
	const FVector Accel = InAccel;
	// Save move parameters.
	const float DeltaTime = ServerData->GetServerMoveDeltaTime(TimeStamp) * CharacterOwner->CustomTimeDilation;

	ServerData->CurrentClientTimeStamp = TimeStamp;
	ServerData->ServerTimeStamp = GetWorld()->TimeSeconds;
	FRotator ViewRot;
	ViewRot.Pitch = FRotator::DecompressAxisFromShort(ViewPitch);
	ViewRot.Yaw = FRotator::DecompressAxisFromShort(ViewYaw);
	ViewRot.Roll = FRotator::DecompressAxisFromByte(ClientRoll);

	if (PC)
	{
		PC->SetControlRotation(ViewRot);
	}

	if (!bServerReadyForClient)
	{
		return;
	}

	// Perform actual movement
	if ((CharacterOwner->GetWorldSettings()->Pauser == NULL) && (DeltaTime > 0.f))
	{
		if (PC)
		{
			PC->UpdateRotation(DeltaTime);
		}

		MoveAutonomous(TimeStamp, DeltaTime, MoveFlags, Accel);
	}

	UE_LOG(LogNetPlayerMovement, Verbose, TEXT("ServerMove Time %f Acceleration %s Position %s DeltaTime %f"),
			TimeStamp, *Accel.ToString(), *CharacterOwner->GetActorLocation().ToString(), DeltaTime);

	ServerMoveHandleClientError(TimeStamp, DeltaTime, Accel, ClientLoc);
}


void UCharacterMovementComponent::ServerMoveHandleClientError(float TimeStamp, float DeltaTime, const FVector& Accel, const FVector& ClientLoc)
{
	if (ClientLoc == FVector(1.f,2.f,3.f)) // first part of double servermove
	{
		return;
	}

	FNetworkPredictionData_Server_Character* ServerData = GetPredictionData_Server_Character();
	check(ServerData);

	// Don't prevent more recent updates from being sent if received this frame.
	// We're going to send out an update anyway, might as well be the most recent one.
	APlayerController* PC = Cast<APlayerController>(CharacterOwner->GetController());
	if( (ServerData->LastUpdateTime != GetWorld()->TimeSeconds) && GetDefault<AGameNetworkManager>()->WithinUpdateDelayBounds(PC, ServerData->LastUpdateTime))
	{
		return;
	}

	const FVector LocDiff = CharacterOwner->GetActorLocation() - ClientLoc;

	// If client has accumulated a noticeable positional error, correct him.
	if (GetDefault<AGameNetworkManager>()->ExceedsAllowablePositionError(LocDiff))
	{
		UPrimitiveComponent* MovementBase = CharacterOwner->GetMovementBase();
		ServerData->PendingAdjustment.NewVel = Velocity;
		ServerData->PendingAdjustment.NewBase = MovementBase;
		ServerData->PendingAdjustment.bBaseRelativePosition = MovementBaseUtility::UseRelativePosition(MovementBase);
		if (ServerData->PendingAdjustment.bBaseRelativePosition)
		{
			ServerData->PendingAdjustment.NewLoc = CharacterOwner->GetActorLocation() - MovementBase->GetComponentLocation();
		}
		else
		{
			ServerData->PendingAdjustment.NewLoc = CharacterOwner->GetActorLocation();
		}

		// Character specific logic
		ServerData->PendingAdjustment.NewRot = CharacterOwner->GetActorRotation();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		static const auto CVarShowCorrections = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("p.NetShowCorrections"));
		static const auto CVarCorrectionLifetime = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("p.NetCorrectionLifetime"));
		if (CVarShowCorrections && CVarShowCorrections->GetValueOnGameThread() != 0)
		{
			UE_LOG(LogNetPlayerMovement, Warning, TEXT("******** Client Error at %f is %f Accel %s LocDiff %s ClientLoc %s, ServerLoc: %s, Base: %s"), TimeStamp, LocDiff.Size(), *Accel.ToString(), *LocDiff.ToString(), *ClientLoc.ToString(), *CharacterOwner->GetActorLocation().ToString(), *GetNameSafe(MovementBase));
			const float DebugLifetime = CVarCorrectionLifetime ? CVarCorrectionLifetime->GetValueOnGameThread() : 1.f;
			DrawDebugCapsule(GetWorld(), CharacterOwner->GetActorLocation(), CharacterOwner->GetSimpleCollisionHalfHeight(), CharacterOwner->GetSimpleCollisionRadius(), FQuat::Identity, FColor(100, 255, 100), true, DebugLifetime);
			DrawDebugCapsule(GetWorld(), ClientLoc                    , CharacterOwner->GetSimpleCollisionHalfHeight(), CharacterOwner->GetSimpleCollisionRadius(), FQuat::Identity, FColor(255, 100, 100), true, DebugLifetime);
		}
#endif

		ServerData->LastUpdateTime = GetWorld()->TimeSeconds;
		ServerData->PendingAdjustment.DeltaTime = DeltaTime;
		ServerData->PendingAdjustment.TimeStamp = TimeStamp;
		ServerData->PendingAdjustment.bAckGoodMove = 0;
	}
	else
	{
		if (GetDefault<AGameNetworkManager>()->ClientAuthorativePosition)
		{
			if (!LocDiff.IsZero())
			{
				// Just set the position. On subsequent moves we will resolve initially overlapping conditions.
				UpdatedComponent->SetWorldLocation(ClientLoc, false);

				// If walking, we need to know where the base and floor are for UpdateBasedMovement() and MoveAlongFloor() to work properly.
				if (MovementMode == MOVE_Walking)
				{
					FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, false);
					SetBase(CurrentFloor.HitResult.Component.Get());
				}

				// Stay in sync with what movement mode is most likely set on the client (now that we know what the floor/base situation is)
				UpdateMovementModeFromAdjustment();
			}			
		}

		// acknowledge receipt of this successful servermove()
		ServerData->PendingAdjustment.TimeStamp = TimeStamp;
		ServerData->PendingAdjustment.bAckGoodMove = 1;
	}
}


void UCharacterMovementComponent::MoveAutonomous
	(
	float ClientTimeStamp,
	float DeltaTime,
	uint8 CompressedFlags,
	const FVector& NewAccel
	)
{
	if (!CharacterOwner)
	{
		return;
	}

	CharacterOwner->UpdateFromCompressedFlags(CompressedFlags);
	CharacterOwner->CheckJumpInput(DeltaTime);

	Acceleration = ConstrainInputAcceleration(NewAccel);
	PerformMovement(DeltaTime);

	CharacterOwner->ClearJumpInput();

	// If not playing root motion, tick animations after physics. We do this here to keep events, notifies, states and transitions in sync with client updates.
	if( !CharacterOwner->bClientUpdating && !CharacterOwner->IsPlayingRootMotion() && CharacterOwner->Mesh )
	{
		CharacterOwner->Mesh->TickPose(DeltaTime);
	}
}


void UCharacterMovementComponent::UpdateMovementModeFromAdjustment()
{
	// Do not turn movement back on if it is disabled
	if (MovementMode == MOVE_None)
	{
		return;
	}

	if (CharacterOwner && UpdatedComponent)
	{
		SetMovementMode(DetermineSimulatedMovementMode());
	}
}


void UCharacterMovementComponent::SendClientAdjustment()
{
	if (!CharacterOwner)
	{
		return;
	}

	FNetworkPredictionData_Server_Character* ServerData = GetPredictionData_Server_Character();
	check(ServerData);

	if (ServerData->PendingAdjustment.TimeStamp <= 0.f)
	{
		return;
	}

	if (ServerData->PendingAdjustment.bAckGoodMove == 1)
	{
		// just notify client this move was received
		CharacterOwner->ClientAckGoodMove(ServerData->PendingAdjustment.TimeStamp);
	}
	else
	{
		if( CharacterOwner->IsPlayingRootMotion() )
		{
			FRotator Rotation = ServerData->PendingAdjustment.NewRot.GetNormalized();
			FVector_NetQuantizeNormal CompressedRotation(Rotation.Pitch / 180.f, Rotation.Yaw / 180.f, Rotation.Roll / 180.f);
			CharacterOwner->ClientAdjustRootMotionPosition
				(
				ServerData->PendingAdjustment.TimeStamp,
				CharacterOwner->GetRootMotionAnimMontageInstance()->Position,
				ServerData->PendingAdjustment.NewLoc,
				CompressedRotation,
				ServerData->PendingAdjustment.NewVel.Z,
				ServerData->PendingAdjustment.NewBase,
				ServerData->PendingAdjustment.NewBase != NULL,
				ServerData->PendingAdjustment.bBaseRelativePosition
				);
		}
		else if (ServerData->PendingAdjustment.NewVel.IsZero())
		{
			CharacterOwner->ClientVeryShortAdjustPosition
				(
				ServerData->PendingAdjustment.TimeStamp,
				ServerData->PendingAdjustment.NewLoc,
				ServerData->PendingAdjustment.NewBase,
				ServerData->PendingAdjustment.NewBase != NULL,
				ServerData->PendingAdjustment.bBaseRelativePosition
				);
		}
		else
		{
			CharacterOwner->ClientAdjustPosition
				(
				ServerData->PendingAdjustment.TimeStamp,
				ServerData->PendingAdjustment.NewLoc,
				ServerData->PendingAdjustment.NewVel,
				ServerData->PendingAdjustment.NewBase,
				ServerData->PendingAdjustment.NewBase != NULL,
				ServerData->PendingAdjustment.bBaseRelativePosition
				);
		}
	}

	ServerData->PendingAdjustment.TimeStamp = 0;
	ServerData->PendingAdjustment.bAckGoodMove = 0;
}


void UCharacterMovementComponent::ClientVeryShortAdjustPosition_Implementation
	(
	float TimeStamp,
	FVector NewLoc,
	UPrimitiveComponent* NewBase,
	bool bHasBase,
	bool bBaseRelativePosition
	)
{
	if (IsValid(CharacterOwner))
	{
		CharacterOwner->ClientAdjustPosition(TimeStamp, NewLoc, FVector::ZeroVector, NewBase, bHasBase, bBaseRelativePosition);
	}
}


void UCharacterMovementComponent::ClientAdjustPosition_Implementation
	(
	float TimeStamp,
	FVector NewLocation,
	FVector NewVelocity,
	UPrimitiveComponent* NewBase,
	bool bHasBase,
	bool bBaseRelativePosition
	)
{
	if (!IsValid(CharacterOwner) || !IsComponentTickEnabled())
	{
		return;
	}


	FNetworkPredictionData_Client_Character* ClientData = GetPredictionData_Client_Character();
	check(ClientData);
	
	// Make sure the base actor exists on this client.
	const bool bUnresolvedBase = bHasBase && (NewBase == NULL);
	if (bUnresolvedBase)
	{
		if (bBaseRelativePosition)
		{
			UE_LOG(LogNetPlayerMovement, Warning, TEXT("ClientAdjustPosition_Implementation could not resolve the new relative movement base actor, ignoring server correction!"));
			return;
		}
		else
		{
			UE_LOG(LogNetPlayerMovement, Verbose, TEXT("ClientAdjustPosition_Implementation could not resolve the new absolute movement base actor, but WILL use the position!"));
		}
	}
	
	// Ack move if it has not expired.
	int32 MoveIndex = ClientData->GetSavedMoveIndex(TimeStamp);
	if( MoveIndex == INDEX_NONE )
	{
		if( ClientData->LastAckedMove.IsValid() )
		{
			UE_LOG(LogNetPlayerMovement, Log,  TEXT("ClientAdjustPosition_Implementation could not find Move for TimeStamp: %f, LastAckedTimeStamp: %f, CurrentTimeStamp: %f"), TimeStamp, ClientData->LastAckedMove->TimeStamp, ClientData->CurrentTimeStamp);
		}
		return;
	}
	ClientData->AckMove(MoveIndex);
		
	//  Received Location is relative to non-world geometry (moving) bases
	if (bBaseRelativePosition)
	{
		NewLocation += NewBase->GetComponentLocation();
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	static const auto CVarShowCorrections = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("p.NetShowCorrections"));
	if (CVarShowCorrections && CVarShowCorrections->GetValueOnGameThread() != 0)
	{
		UE_LOG(LogNetPlayerMovement, Warning, TEXT("******** ClientAdjustPosition Time %f velocity %s position %s NewBase: %s SavedMoves %d"), TimeStamp, *NewVelocity.ToString(), *NewLocation.ToString(), *GetNameSafe(NewBase), ClientData->SavedMoves.Num());
		static const auto CVarCorrectionLifetime = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("p.NetCorrectionLifetime"));
		const float DebugLifetime = CVarCorrectionLifetime ? CVarCorrectionLifetime->GetValueOnGameThread() : 1.f;
		DrawDebugCapsule(GetWorld(), CharacterOwner->GetActorLocation()	, CharacterOwner->GetSimpleCollisionHalfHeight(), CharacterOwner->GetSimpleCollisionRadius(), FQuat::Identity, FColor(255, 100, 100), true, DebugLifetime);
		DrawDebugCapsule(GetWorld(), NewLocation						, CharacterOwner->GetSimpleCollisionHalfHeight(), CharacterOwner->GetSimpleCollisionRadius(), FQuat::Identity, FColor(100, 255, 100), true, DebugLifetime);
	}
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	// Trust the server's positioning.
	UpdatedComponent->SetWorldLocation(NewLocation, false);	

	// Set base component
	UPrimitiveComponent* FinalBase = NewBase;
	if (bUnresolvedBase)
	{
		check(NewBase == NULL);
		check(!bBaseRelativePosition);
		
		// We had an unresolved base from the server
		// If walking, we'd like to continue walking if possible, to avoid falling for a frame, so try to find a base where we moved to.
		if (MovementMode == MOVE_Walking)
		{
			FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, false);
			FinalBase = CurrentFloor.HitResult.Component.Get();
		}
	}

	// Set base component
	SetBase(FinalBase);

	// Update movement mode (base may have changed)
	UpdateMovementModeFromAdjustment();
	Velocity = NewVelocity;
	UpdateComponentVelocity();

	ClientData->bUpdatePosition = true;
}

void UCharacterMovementComponent::ClientAdjustRootMotionPosition_Implementation(float TimeStamp, float ServerMontageTrackPosition, FVector ServerLoc, FVector_NetQuantizeNormal ServerRotation, float ServerVelZ, class UPrimitiveComponent * ServerBase, bool bHasBase, bool bBaseRelativePosition)
{
	if (!IsValid(CharacterOwner) || !IsComponentTickEnabled())
	{
		return;
	}

	// Call ClientAdjustPosition first. This will Ack the move if it's not outdated.
	CharacterOwner->ClientAdjustPosition(TimeStamp, ServerLoc, FVector(0.f, 0.f, ServerVelZ), ServerBase, bHasBase, bBaseRelativePosition);
	
	FNetworkPredictionData_Client_Character* ClientData = GetPredictionData_Client_Character();
	check(ClientData);

	// If this adjustment wasn't acknowledged (because outdated), then abort.
	if( !ClientData->LastAckedMove.IsValid() || (ClientData->LastAckedMove->TimeStamp != TimeStamp) )
	{
		return;
	}

	// We're going to replay Root Motion. This is relative to the Pawn's rotation, so we need to reset that as well.
	FRotator DecompressedRot(ServerRotation.X * 180.f, ServerRotation.Y * 180.f, ServerRotation.Z * 180.f);
	CharacterOwner->SetActorRotation(DecompressedRot);
	const FVector ServerLocation(ServerLoc);
	UE_LOG(LogRootMotion, Log,  TEXT("ClientAdjustRootMotionPosition_Implementation TimeStamp: %f, ServerMontageTrackPosition: %f, ServerLocation: %s, ServerRotation: %s, ServerVelZ: %f, ServerBase: %s"),
		TimeStamp, ServerMontageTrackPosition, *ServerLocation.ToCompactString(), *DecompressedRot.ToCompactString(), ServerVelZ, *GetNameSafe(ServerBase) );

	// DEBUG - get some insight on where errors came from
	if( false )
	{
		const FVector DeltaLocation = ServerLocation - ClientData->LastAckedMove->SavedLocation;
		const FRotator DeltaRotation = (DecompressedRot - ClientData->LastAckedMove->SavedRotation).GetNormalized();
		const float DeltaTrackPosition = (ServerMontageTrackPosition - ClientData->LastAckedMove->RootMotionTrackPosition);
		const float DeltaVelZ = (ServerVelZ - ClientData->LastAckedMove->SavedVelocity.Z);

		UE_LOG(LogRootMotion, Log,  TEXT("\tErrors DeltaLocation: %s, DeltaRotation: %s, DeltaTrackPosition: %f"),
			*DeltaLocation.ToCompactString(), *DeltaRotation.ToCompactString(), DeltaTrackPosition );
	}

	// Server disagrees with Client on the Root Motion AnimMontage Track position.
	if( CharacterOwner->bClientResimulateRootMotion || (ServerMontageTrackPosition != ClientData->LastAckedMove->RootMotionTrackPosition) )
	{
		UE_LOG(LogRootMotion, Warning,  TEXT("\tServer disagrees with Client's track position!! ServerTrackPosition: %f, ClientTrackPosition: %f, DeltaTrackPosition: %f. TimeStamp: %f"),
			ServerMontageTrackPosition, ClientData->LastAckedMove->RootMotionTrackPosition, (ServerMontageTrackPosition - ClientData->LastAckedMove->RootMotionTrackPosition), TimeStamp);

		// Not much we can do there unfortunately, just jump to server's track position.
		FAnimMontageInstance * RootMotionMontageInstance = CharacterOwner->GetRootMotionAnimMontageInstance();
		if( RootMotionMontageInstance )
		{
			RootMotionMontageInstance->Position = ServerMontageTrackPosition;
			CharacterOwner->bClientResimulateRootMotion = true;
		}
	}
}

void UCharacterMovementComponent::ClientAckGoodMove_Implementation(float TimeStamp)
{
	if (!IsValid(CharacterOwner) || !IsComponentTickEnabled())
	{
		return;
	}

	FNetworkPredictionData_Client_Character* ClientData = GetPredictionData_Client_Character();
	check(ClientData);


	// Ack move if it has not expired.
	int32 MoveIndex = ClientData->GetSavedMoveIndex(TimeStamp);
	if( MoveIndex == INDEX_NONE )
	{
		if( ClientData->LastAckedMove.IsValid() )
		{
			UE_LOG(LogNetPlayerMovement, Warning, TEXT("ClientAckGoodMove_Implementation could not find Move for TimeStamp: %f, LastAckedTimeStamp: %f, CurrentTimeStamp: %f"), TimeStamp, ClientData->LastAckedMove->TimeStamp, ClientData->CurrentTimeStamp);
		}
		return;
	}
	ClientData->AckMove(MoveIndex);
}

void UCharacterMovementComponent::CapsuleTouched( AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex )
{
	check(bEnablePhysicsInteraction);

	if (OtherComp != NULL && OtherComp->IsAnySimulatingPhysics())
	{
		const FVector OtherLoc = OtherComp->GetComponentLocation();
		const FVector Loc = UpdatedComponent->GetComponentLocation();
		FVector ImpulseDir = FVector(OtherLoc.X - Loc.X, OtherLoc.Y - Loc.Y, 0.25f).SafeNormal();
		ImpulseDir = (ImpulseDir + Velocity.SafeNormal2D()) * 0.5f;
		ImpulseDir.Normalize();

		FName BoneName = NAME_None;
		if (OtherBodyIndex != INDEX_NONE)
		{
			BoneName = ((USkinnedMeshComponent*)OtherComp)->GetBoneName(OtherBodyIndex);
		}

		float TouchForceFactorModified = TouchForceFactor;

		if ( bTouchForceScaledToMass )
		{
			FBodyInstance* BI = OtherComp->GetBodyInstance(BoneName);
			TouchForceFactorModified *= BI ? BI->GetBodyMass() : 1.0f;
		}

		float ImpulseStrength = FMath::Clamp(Velocity.Size2D() * TouchForceFactorModified, 
			MinTouchForce > 0.0f ? MinTouchForce : -FLT_MAX, 
			MaxTouchForce > 0.0f ? MaxTouchForce : FLT_MAX);

		FVector Impulse = ImpulseDir * ImpulseStrength;

		OtherComp->AddImpulse(Impulse, BoneName);
	}
}

void UCharacterMovementComponent::SetAvoidanceGroup(int32 GroupFlags)
{
	AvoidanceGroup.SetFlagsDirectly(GroupFlags);
}

void UCharacterMovementComponent::SetGroupsToAvoid(int32 GroupFlags)
{
	GroupsToAvoid.SetFlagsDirectly(GroupFlags);
}

void UCharacterMovementComponent::SetGroupsToIgnore(int32 GroupFlags)
{
	GroupsToIgnore.SetFlagsDirectly(GroupFlags);
}

void UCharacterMovementComponent::ApplyRepulsionForce( float DeltaTime )
{
	FCollisionQueryParams QueryParams;
	QueryParams.bReturnFaceIndex = false;
	QueryParams.bReturnPhysicalMaterial = false;

	const float CapsuleRadius = UpdatedComponent->GetCollisionShape().GetCapsuleRadius();
	const float CapsuleHalfHeight = UpdatedComponent->GetCollisionShape().GetCapsuleHalfHeight();
	const float RepulsionForceRadius = CapsuleRadius * 1.2f;
	const float StopBodyDistance = 2.5f;
	
	if (RepulsionForce > 0.0f)
	{
		const TArray<FOverlapInfo>& Overlaps = UpdatedComponent->GetOverlapInfos();
		
		const FVector MyLocation = UpdatedComponent->GetComponentLocation();

		for (int32 i=0; i < Overlaps.Num(); ++i)
		{
			const FOverlapInfo& Overlap = Overlaps[i];

			UPrimitiveComponent* OverlapComp = Overlap.Component.Get();
			if (!OverlapComp || OverlapComp->Mobility < EComponentMobility::Movable) { continue; }

			FName BoneName = NAME_None;
			if (Overlap.BodyIndex != INDEX_NONE && OverlapComp->IsA(USkinnedMeshComponent::StaticClass()))
			{
				BoneName = ((USkinnedMeshComponent*)OverlapComp)->GetBoneName(Overlap.BodyIndex);
			}

			// Use the body instead of the component for cases where we have multi-body overlaps enabled
			FBodyInstance* OverlapBody = OverlapComp->GetBodyInstance(BoneName);

			if (!OverlapBody)
			{
				UE_LOG(LogCharacterMovement, Warning, TEXT("%s could not find overlap body for bone %s"), *GetName(), *BoneName.ToString());
				continue;
			}

			// Early out if this is not a destructible and the body is not simulated
			bool bIsCompDestructible = OverlapComp->IsA(UDestructibleComponent::StaticClass());
			if (!bIsCompDestructible && !OverlapBody->IsInstanceSimulatingPhysics())
			{
				continue;
			}


			FTransform BodyTransform = OverlapBody->GetUnrealWorldTransform();
			
			FVector BodyVelocity = OverlapBody->GetUnrealWorldVelocity();
			FVector BodyLocation = BodyTransform.GetLocation();

			// Trace to get the hit location on the capsule
			FHitResult Hit;
			bool bHasHit = UpdatedComponent->LineTraceComponent(Hit, BodyLocation, 
																FVector(MyLocation.X, MyLocation.Y, BodyLocation.Z),
																QueryParams);

			FVector HitLoc = Hit.ImpactPoint;
			bool bIsPenetrating = Hit.bStartPenetrating || Hit.PenetrationDepth > 2.5f;
			
			// If we didn't hit the capsule, we're inside the capsule
			if(!bHasHit) 
			{ 
				HitLoc = BodyLocation; 
				bIsPenetrating = true;
			}
			
			const float DistanceNow = (HitLoc - BodyLocation).SizeSquared2D();
			const float DistanceLater = (HitLoc - (BodyLocation + BodyVelocity * DeltaTime)).SizeSquared2D();

			if (BodyLocation.SizeSquared() > 0.1f && bHasHit && DistanceNow < StopBodyDistance && !bIsPenetrating)
			{
				OverlapBody->SetLinearVelocity(FVector(0.0f, 0.0f, 0.0f), false);
			}
			else if (DistanceLater <= DistanceNow || bIsPenetrating)
			{
				FVector ForceCenter(MyLocation.X, MyLocation.Y, bHasHit ? HitLoc.Z : MyLocation.Z);

				if (!bHasHit)
				{
					ForceCenter.Z = FMath::Clamp(BodyLocation.Z, MyLocation.Z - CapsuleHalfHeight, MyLocation.Z + CapsuleHalfHeight);
				}

				OverlapBody->AddRadialForceToBody(ForceCenter, RepulsionForceRadius, RepulsionForce * Mass, ERadialImpulseFalloff::RIF_Constant);
			}
		}
	}
}

FNetworkPredictionData_Client_Character::FNetworkPredictionData_Client_Character()
	: ClientUpdateTime(0.f)
	, CurrentTimeStamp(0.f)
	, PendingMove(NULL)
	, LastAckedMove(NULL)
	, MaxFreeMoveCount(32)
	, MaxSavedMoveCount(96)
	, bUpdatePosition(false)
	, bSmoothNetUpdates(false)
	, MeshTranslationOffset(ForceInitToZero)
	, MaxSmoothNetUpdateDist(0.f)
	, NoSmoothNetUpdateDist(0.f)
	, SmoothNetUpdateTime(0.f)
	, MaxResponseTime(0.f)
{
	bSmoothNetUpdates = true;
	MaxSmoothNetUpdateDist = 84.0;
	NoSmoothNetUpdateDist = 128.0;
	SmoothNetUpdateTime = 0.125;

	MaxResponseTime = 0.125f;
}


FNetworkPredictionData_Client_Character::~FNetworkPredictionData_Client_Character()
{
	SavedMoves.Empty();
	FreeMoves.Empty();
	PendingMove = NULL;
	LastAckedMove = NULL;
}


FSavedMovePtr FNetworkPredictionData_Client_Character::CreateSavedMove()
{
	if (SavedMoves.Num() >= MaxSavedMoveCount)
	{
		UE_LOG(LogNetPlayerMovement, Warning, TEXT("CreateSavedMove: Hit limit of %d saved moves (timing out or very bad ping?)"), SavedMoves.Num());
		// Free all saved moves
		for (int32 i=0; i < SavedMoves.Num(); i++)
		{
			FreeMove(SavedMoves[i]);
		}
		SavedMoves.Reset();
	}

	if (FreeMoves.Num() == 0)
	{
		// No free moves, allocate a new one.
		FSavedMovePtr NewMove = AllocateNewMove();
		check(NewMove.IsValid());
		NewMove->Clear();
		return NewMove;
	}
	else
	{
		// Pull from the free pool
		FSavedMovePtr FirstFree = FreeMoves.Pop();
		FirstFree->Clear();
		return FirstFree;
	}
}


FSavedMovePtr FNetworkPredictionData_Client_Character::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_Character());
}


void FNetworkPredictionData_Client_Character::FreeMove(const FSavedMovePtr& Move)
{
	if (Move.IsValid())
	{
		// Only keep a pool of a limited number of moves.
		if (FreeMoves.Num() < MaxFreeMoveCount)
		{
			FreeMoves.Push(Move);
		}

		// Shouldn't keep a reference to the move on the free list.
		if (PendingMove == Move)
		{
			PendingMove = NULL;
		}
		if( LastAckedMove == Move )
		{
			LastAckedMove = NULL;
		}
	}
}

int32 FNetworkPredictionData_Client_Character::GetSavedMoveIndex(float TimeStamp) const
{
	if( SavedMoves.Num() > 0 )
	{
		// If LastAckedMove isn't using an old TimeStamp (before reset), we can prevent the iteration if incoming TimeStamp is outdated
		if( LastAckedMove.IsValid() && !LastAckedMove->bOldTimeStampBeforeReset && (TimeStamp <= LastAckedMove->TimeStamp) )
		{
			return INDEX_NONE;
		}

		// Otherwise see if we can find this move.
		for (int32 Index=0; Index<SavedMoves.Num(); Index++)
		{
			const FSavedMovePtr & CurrentMove = SavedMoves[Index];
			if( CurrentMove->TimeStamp == TimeStamp )
			{
				return Index;
			}
		}
	}
	return INDEX_NONE;
}

void FNetworkPredictionData_Client_Character::AckMove(int32 AckedMoveIndex) 
{
	// It is important that we know the move exists before we go deleting outdated moves.
	// Timestamps are not guaranteed to be increasing order all the time, since they can be reset!
	if( AckedMoveIndex != INDEX_NONE )
	{
		// Keep reference to LastAckedMove
		const FSavedMovePtr& AckedMove = SavedMoves[AckedMoveIndex];
		UE_LOG(LogNetPlayerMovement, VeryVerbose, TEXT("AckedMove Index: %2d (%2d moves). TimeStamp: %f, CurrentTimeStamp: %f"), AckedMoveIndex, SavedMoves.Num(), AckedMove->TimeStamp, CurrentTimeStamp);
		if( LastAckedMove.IsValid() )
		{
			FreeMove(LastAckedMove);
		}
		LastAckedMove = AckedMove;

		// Free expired moves.
		for(int32 MoveIndex=0; MoveIndex<AckedMoveIndex; MoveIndex++)
		{
			const FSavedMovePtr& Move = SavedMoves[MoveIndex];
			FreeMove(Move);
		}

		// And finally cull all of those, so only the unacknowledged moves remain in SavedMoves.
		SavedMoves.RemoveAt(0, AckedMoveIndex + 1);
	}
}

FNetworkPredictionData_Server_Character::FNetworkPredictionData_Server_Character()
	: PendingAdjustment()
	, CurrentClientTimeStamp(0.f)
	, LastUpdateTime(0.f)
	, MaxResponseTime(0.125f)
{
}


FNetworkPredictionData_Server_Character::~FNetworkPredictionData_Server_Character()
{
}


float FNetworkPredictionData_Server_Character::GetServerMoveDeltaTime(float TimeStamp) const
{
	const float DeltaTime = FMath::Min(MaxResponseTime, TimeStamp - CurrentClientTimeStamp);
	return DeltaTime;
}

static uint32 s_savedMoveCount = 0;

FSavedMove_Character::FSavedMove_Character()
{
	AccelMagThreshold = 1.f;
	AccelDotThreshold = 0.9f;
	s_savedMoveCount++;
}

FSavedMove_Character::~FSavedMove_Character()
{
	s_savedMoveCount--;
}

void FSavedMove_Character::Clear()
{
	bPressedJump = false;
	bWantsToCrouch = false;
	bForceMaxAccel = false;
	bOldTimeStampBeforeReset = false;

	TimeStamp = 0.f;
	DeltaTime = 0.f;
	CustomTimeDilation = 1.0f;

	StartLocation = FVector::ZeroVector;
	StartRelativeLocation = FVector::ZeroVector;
	StartVelocity = FVector::ZeroVector;
	StartFloor = FFindFloorResult();
	StartRotation = FRotator::ZeroRotator;
	StartControlRotation = FRotator::ZeroRotator;
	SavedLocation = FVector::ZeroVector;
	SavedRotation = FRotator::ZeroRotator;
	SavedRelativeLocation = FVector::ZeroVector;
	Acceleration = FVector::ZeroVector;
	ControlRotation = FRotator::ZeroRotator;

	StartBase = NULL;
	EndBase = NULL;

	RootMotionMontage = NULL;
	RootMotionTrackPosition = 0.f;
	RootMotionMovement.Clear();
}


void FSavedMove_Character::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character & ClientData)
{
	DeltaTime = InDeltaTime;
	
	SetInitialPosition(Character);

	AccelMag = NewAccel.Size();
	AccelNormal = (AccelMag > SMALL_NUMBER ? NewAccel / AccelMag : FVector::ZeroVector);
	Acceleration = NewAccel;

	bPressedJump = Character->bPressedJump;
	bWantsToCrouch = Character->CharacterMovement->bWantsToCrouch;
	bForceMaxAccel = Character->CharacterMovement->bForceMaxAccel;
	
	TimeStamp = ClientData.CurrentTimeStamp;
}

void FSavedMove_Character::SetInitialPosition(ACharacter* Character)
{
	StartLocation = Character->GetActorLocation();
	StartRotation = Character->GetActorRotation();
	StartVelocity = Character->CharacterMovement->Velocity;
	UPrimitiveComponent* const MovementBase = Character->GetMovementBase();
	StartBase = MovementBase;
	StartFloor = Character->CharacterMovement->CurrentFloor;
	CustomTimeDilation = Character->CustomTimeDilation;

	if (Character->Controller)
	{
		StartControlRotation = Character->Controller->GetControlRotation();
	}

	if (MovementBaseUtility::UseRelativePosition(MovementBase))
	{
		StartRelativeLocation = StartLocation - MovementBase->GetComponentLocation();
	}
}

void FSavedMove_Character::PostUpdate(ACharacter* Character)
{
	SavedLocation = Character->GetActorLocation();
	SavedRotation = Character->GetActorRotation();
	UPrimitiveComponent* const MovementBase = Character->GetMovementBase();
	EndBase = MovementBase;
	if (MovementBaseUtility::UseRelativePosition(MovementBase))
	{
		SavedRelativeLocation = SavedLocation - MovementBase->GetComponentLocation();
	}

	ControlRotation = Character->GetControlRotation();

	if( Character )
	{
		const FAnimMontageInstance * RootMotionMontageInstance = Character->GetRootMotionAnimMontageInstance();
		if( RootMotionMontageInstance )
		{
			RootMotionMontage = RootMotionMontageInstance->Montage;
			RootMotionTrackPosition = RootMotionMontageInstance->Position;
			RootMotionMovement = Character->ClientRootMotionParams;
		}
	}
}

bool FSavedMove_Character::IsImportantMove(const FSavedMovePtr& LastAckedMove)
{
	// Check if any important movement flags have changed status.
	if( (bPressedJump != LastAckedMove->bPressedJump)|| (bWantsToCrouch != LastAckedMove->bWantsToCrouch) )
	{
		return true;
	}

	// check if acceleration has changed significantly
	if( Acceleration != LastAckedMove->Acceleration ) 
	{
		// Compare magnitude and orientation
		if( (FMath::Abs(AccelMag - LastAckedMove->AccelMag) > AccelMagThreshold) || ((AccelNormal | LastAckedMove->AccelNormal) < AccelDotThreshold) )
		{
			return true;
		}
	}
	return false;
}

FVector FSavedMove_Character::GetStartLocation()
{
	if (MovementBaseUtility::UseRelativePosition(StartBase.Get()))
	{
		return StartBase->GetComponentLocation() + StartRelativeLocation;
	}

	return StartLocation;
}

bool FSavedMove_Character::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta)
{
	// Cannot combine moves which contain root motion for now.
	// @fixme laurent - we should be able to combine most of them though, but current scheme of resetting pawn location and resimulating forward doesn't work.
	// as we don't want to tick montage twice (so we don't fire events twice). So we need to rearchitecture this so we tick only the second part of the move, and reuse the first part.
	if( (RootMotionMontage != NULL) || (NewMove->RootMotionMontage != NULL) )
	{
		return false;
	}

	if (NewMove->Acceleration.IsZero())
	{
		return Acceleration.IsZero()
			&& StartVelocity.IsZero()
			&& NewMove->StartVelocity.IsZero()
			&& !bPressedJump && !NewMove->bPressedJump
			&& bWantsToCrouch == NewMove->bWantsToCrouch
			&& (StartBase == NewMove->StartBase)
			&& (CustomTimeDilation == NewMove->CustomTimeDilation);
	}
	else
	{
		return (NewMove->DeltaTime + DeltaTime < MaxDelta)
			&& !bPressedJump && !NewMove->bPressedJump
			&& bWantsToCrouch == NewMove->bWantsToCrouch
			&& (StartBase == NewMove->StartBase)
			&& ((AccelNormal | NewMove->AccelNormal) > 0.99f)
			&& (CustomTimeDilation == NewMove->CustomTimeDilation);
	}
}

void FSavedMove_Character::PrepMoveFor(ACharacter* Character)
{
	if( RootMotionMontage != NULL )
	{
		// If we need to resimulate Root Motion, then do so.
		if( Character->bClientResimulateRootMotion )
		{
			// Make sure RootMotion montage matches what we are playing now.
			FAnimMontageInstance * RootMotionMontageInstance = Character->GetRootMotionAnimMontageInstance();
			if( RootMotionMontageInstance && (RootMotionMontage == RootMotionMontageInstance->Montage) )
			{
				RootMotionMovement.Clear();
				RootMotionMontageInstance->SimulateAdvance(DeltaTime, RootMotionMontageInstance->Position, RootMotionMovement);
				RootMotionTrackPosition = RootMotionMontageInstance->Position;
			}
		}
		Character->CharacterMovement->RootMotionParams = RootMotionMovement;
	}

	Character->CharacterMovement->bForceMaxAccel = bForceMaxAccel;
}

void FSavedMove_Character::ResetMoveFor(ACharacter* Character)
{
	SavedLocation = Character->GetActorLocation();
	SavedRotation = Character->GetActorRotation();
	UPrimitiveComponent* const MovementBase = Character->GetMovementBase();
	EndBase = MovementBase;
	if (MovementBaseUtility::UseRelativePosition(MovementBase))
	{
		SavedRelativeLocation = Character->GetActorLocation() - MovementBase->GetComponentLocation();
	}	
}

uint8 FSavedMove_Character::GetCompressedFlags()
{
	uint8 Result = 0;

	if (bPressedJump)
	{
		Result |= FLAG_JumpPressed;
	}

	if (bWantsToCrouch)
	{
		Result |= FLAG_WantsToCrouch;
	}

	return Result;
}



