// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

//////////////// PRIMITIVECOMPONENT ///////////////

bool UPrimitiveComponent::ApplyRigidBodyState(const FRigidBodyState& NewState, const FRigidBodyErrorCorrection& ErrorCorrection, FVector& OutDeltaPos, FName BoneName)
{
	bool bRestoredState = true;

	FBodyInstance* BI = GetBodyInstance(BoneName);
	if (BI && BI->IsInstanceSimulatingPhysics())
	{
		// failure cases
		const float QuatSizeSqr = NewState.Quaternion.SizeSquared();
		if (QuatSizeSqr < KINDA_SMALL_NUMBER)
		{
			UE_LOG(LogPhysics, Warning, TEXT("Invalid zero quaternion set for body. (%s:%s)"), *GetName(), *BoneName.ToString());
			return bRestoredState;
		}
		else if (FMath::Abs(QuatSizeSqr - 1.f) > KINDA_SMALL_NUMBER)
		{
			UE_LOG(LogPhysics, Warning, TEXT("Quaternion (%f %f %f %f) with non-unit magnitude detected. (%s:%s)"), 
				NewState.Quaternion.X, NewState.Quaternion.Y, NewState.Quaternion.Z, NewState.Quaternion.W, *GetName(), *BoneName.ToString() );
			return bRestoredState;
		}

		FRigidBodyState CurrentState;
		GetRigidBodyState(CurrentState, BoneName);

		const bool bShouldSleep = (NewState.Flags & ERigidBodyFlags::Sleeping) != 0;

		/////// POSITION CORRECTION ///////

		// Find out how much of a correction we are making
		const FVector DeltaPos = NewState.Position - CurrentState.Position;
		const float DeltaMagSq = DeltaPos.SizeSquared();
		const float BodyLinearSpeedSq = CurrentState.LinVel.SizeSquared();

		// Snap position by default (big correction, or we are moving too slowly)
		FVector UpdatedPos = NewState.Position;
		FVector FixLinVel = FVector::ZeroVector;

		// If its a small correction and velocity is above threshold, only make a partial correction, 
		// and calculate a velocity that would fix it over 'fixTime'.
		if (DeltaMagSq < ErrorCorrection.LinearDeltaThresholdSq  &&
			BodyLinearSpeedSq >= ErrorCorrection.BodySpeedThresholdSq)
		{
			UpdatedPos = FMath::Lerp(CurrentState.Position, NewState.Position, ErrorCorrection.LinearInterpAlpha);
			FixLinVel = (NewState.Position - UpdatedPos) * ErrorCorrection.LinearRecipFixTime;
		}

		// Get the linear correction
		OutDeltaPos = UpdatedPos - CurrentState.Position;

		/////// ORIENTATION CORRECTION ///////
		// Get quaternion that takes us from old to new
		const FQuat InvCurrentQuat = CurrentState.Quaternion.Inverse();
		const FQuat DeltaQuat = NewState.Quaternion * InvCurrentQuat;

		FVector DeltaAxis;
		float DeltaAng;	// radians
		DeltaQuat.ToAxisAndAngle(DeltaAxis, DeltaAng);
		DeltaAng = FMath::UnwindRadians(DeltaAng);

		// Snap rotation by default (big correction, or we are moving too slowly)
		FQuat UpdatedQuat = NewState.Quaternion;
		FVector FixAngVel = FVector::ZeroVector; // degrees per second
		
		// If the error is small, and we are moving, try to move smoothly to it
		if (FMath::Abs(DeltaAng) < ErrorCorrection.AngularDeltaThreshold )
		{
			UpdatedQuat = FMath::Lerp(CurrentState.Quaternion, NewState.Quaternion, ErrorCorrection.AngularInterpAlpha);
			FixAngVel = DeltaAxis.SafeNormal() * FMath::RadiansToDegrees(DeltaAng) * (1.f - ErrorCorrection.AngularInterpAlpha) * ErrorCorrection.AngularRecipFixTime;
		}

		/////// BODY UPDATE ///////
		BI->SetBodyTransform(FTransform(UpdatedQuat, UpdatedPos), true);
		BI->SetLinearVelocity(NewState.LinVel + FixLinVel, false);
		BI->SetAngularVelocity(NewState.AngVel + FixAngVel, false);

		// state is restored when no velocity corrections are required
		bRestoredState = (FixLinVel.SizeSquared() < KINDA_SMALL_NUMBER) && (FixAngVel.SizeSquared() < KINDA_SMALL_NUMBER);
	
		/////// SLEEP UPDATE ///////
		const bool bIsAwake = BI->IsInstanceAwake();
		if (bIsAwake && (bShouldSleep && bRestoredState))
		{
			BI->PutInstanceToSleep();
		}
		else if (!bIsAwake)
		{
			BI->WakeInstance();
		}
	}

	return bRestoredState;
}

bool UPrimitiveComponent::ConditionalApplyRigidBodyState(FRigidBodyState& UpdatedState, const FRigidBodyErrorCorrection& ErrorCorrection, FVector& OutDeltaPos, FName BoneName)
{
	bool bUpdated = false;

	// force update if simulation is sleeping on server
	if ((UpdatedState.Flags & ERigidBodyFlags::Sleeping) && RigidBodyIsAwake(BoneName))
	{
		UpdatedState.Flags |= ERigidBodyFlags::NeedsUpdate;	
	}

	if (UpdatedState.Flags & ERigidBodyFlags::NeedsUpdate)
	{
		const bool bRestoredState = ApplyRigidBodyState(UpdatedState, ErrorCorrection, OutDeltaPos, BoneName);
		if (bRestoredState)
		{
			UpdatedState.Flags &= ~ERigidBodyFlags::NeedsUpdate;
		}

		bUpdated = true;
	}

	return bUpdated;
}

bool UPrimitiveComponent::GetRigidBodyState(FRigidBodyState& OutState, FName BoneName)
{
	FBodyInstance* BI = GetBodyInstance(BoneName);
	if (BI && BI->IsInstanceSimulatingPhysics())
	{
		FTransform BodyTM = BI->GetUnrealWorldTransform();
		OutState.Position = BodyTM.GetTranslation();
		OutState.Quaternion = BodyTM.GetRotation();
		OutState.LinVel = BI->GetUnrealWorldVelocity();
		OutState.AngVel = BI->GetUnrealWorldAngularVelocity();
		OutState.Flags = (BI->IsInstanceAwake() ? ERigidBodyFlags::None : ERigidBodyFlags::Sleeping);
		return true;
	}

	return false;
}

const struct FWalkableSlopeOverride& UPrimitiveComponent::GetWalkableSlopeOverride() const
{
	return BodyInstance.GetWalkableSlopeOverride();
}

void UPrimitiveComponent::SetSimulatePhysics(bool bSimulate)
{
	BodyInstance.SetInstanceSimulatePhysics(bSimulate);
}

void UPrimitiveComponent::AddImpulse(FVector Impulse, FName BoneName, bool bVelChange)
{
	FBodyInstance* BI = GetBodyInstance(BoneName);
	if(BI)
	{
		BI->AddImpulse(Impulse, bVelChange);
	}
}

void UPrimitiveComponent::AddImpulseAtLocation(FVector Impulse, FVector Location, FName BoneName)
{
	FBodyInstance* BI = GetBodyInstance(BoneName);
	if(BI)
	{
		BI->AddImpulseAtPosition(Impulse, Location);
	}
}

void UPrimitiveComponent::AddRadialImpulse(FVector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff, bool bVelChange)
{
	if(bIgnoreRadialImpulse)
	{
		return;
	}

	BodyInstance.AddRadialImpulseToBody(Origin, Radius, Strength, Falloff, bVelChange);
}


void UPrimitiveComponent::AddForce(FVector Force, FName BoneName)
{
	FBodyInstance* BI = GetBodyInstance(BoneName);
	if(BI)
	{
		BI->AddForce(Force);
	}
}

void UPrimitiveComponent::AddForceAtLocation(FVector Force, FVector Location, FName BoneName)
{
	FBodyInstance* BI = GetBodyInstance(BoneName);
	if(BI)
	{
		BI->AddForceAtPosition(Force, Location);
	}
}

void UPrimitiveComponent::AddRadialForce(FVector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff)
{
	if(bIgnoreRadialForce)
	{
		return;
	}

	BodyInstance.AddRadialForceToBody(Origin, Radius, Strength, Falloff);
}

void UPrimitiveComponent::AddTorque(FVector Torque, FName BoneName)
{
	FBodyInstance* BI = GetBodyInstance(BoneName);
	if(BI)
	{
		BI->AddTorque(Torque);
	}
}

void UPrimitiveComponent::SetPhysicsLinearVelocity(FVector NewVel, bool bAddToCurrent, FName BoneName)
{
	FBodyInstance* BI = GetBodyInstance(BoneName);
	if(BI != NULL)
	{
		BI->SetLinearVelocity(NewVel, bAddToCurrent);
	}
}

FVector UPrimitiveComponent::GetPhysicsLinearVelocity(FName BoneName)
{
	FBodyInstance* BI = GetBodyInstance(BoneName);
	if(BI != NULL)
	{
		return BI->GetUnrealWorldVelocity();
	}
	return FVector(0,0,0);
}

void UPrimitiveComponent::SetAllPhysicsLinearVelocity(FVector NewVel,bool bAddToCurrent)
{
	SetPhysicsLinearVelocity(NewVel, bAddToCurrent, NAME_None);
}

void UPrimitiveComponent::SetPhysicsAngularVelocity(FVector NewAngVel, bool bAddToCurrent, FName BoneName)
{
	FBodyInstance* const BI = GetBodyInstance(BoneName);
	if(BI != NULL)
	{
		BI->SetAngularVelocity(NewAngVel, bAddToCurrent);
	}
}

FVector UPrimitiveComponent::GetPhysicsAngularVelocity(FName BoneName)
{
	FBodyInstance* const BI = GetBodyInstance(BoneName);
	if(BI != NULL)
	{
		return BI->GetUnrealWorldAngularVelocity();
	}
	return FVector(0,0,0);
}

void UPrimitiveComponent::SetAllPhysicsAngularVelocity(FVector const& NewAngVel, bool bAddToCurrent)
{
	SetPhysicsAngularVelocity(NewAngVel, bAddToCurrent, NAME_None); 
}


void UPrimitiveComponent::SetAllPhysicsPosition(FVector NewPos)
{
	SetWorldLocation(NewPos, NAME_None);
}


void UPrimitiveComponent::SetAllPhysicsRotation(FRotator NewRot)
{
	SetWorldRotation(NewRot, NAME_None);
}


void UPrimitiveComponent::WakeRigidBody(FName BoneName)
{
	FBodyInstance* BI = GetBodyInstance(BoneName);
	if(BI)
	{
		BI->WakeInstance();
	}
}

void UPrimitiveComponent::WakeAllRigidBodies()
{
	WakeRigidBody(NAME_None);
}

void UPrimitiveComponent::SetEnableGravity(bool bGravityEnabled)
{
	BodyInstance.SetEnableGravity(bGravityEnabled);
}

bool UPrimitiveComponent::IsGravityEnabled() const
{
	return BodyInstance.bEnableGravity;
}

void UPrimitiveComponent::SetLinearDamping(float InDamping)
{
	BodyInstance.LinearDamping = InDamping;
	BodyInstance.UpdateDampingProperties();
}

float UPrimitiveComponent::GetLinearDamping() const
{
	return BodyInstance.LinearDamping;
}

void UPrimitiveComponent::SetAngularDamping(float InDamping)
{
	BodyInstance.AngularDamping = InDamping;
	BodyInstance.UpdateDampingProperties();
}

float UPrimitiveComponent::GetAngularDamping() const
{
	return BodyInstance.AngularDamping;
}

float UPrimitiveComponent::GetMass() const
{
	if (BodyInstance.IsValidBodyInstance())
	{
		return BodyInstance.GetBodyMass();
	}

	return 0.0f;
}

float UPrimitiveComponent::CalculateMass() const
{
	if (BodyInstance.BodySetup.IsValid())
	{
		return BodyInstance.BodySetup->CalculateMass(this);
	}
	return 0.0f;
}

void UPrimitiveComponent::PutRigidBodyToSleep(FName BoneName)
{
	FBodyInstance* BI = GetBodyInstance(BoneName);
	if(BI)
	{
		BI->PutInstanceToSleep();
	}
}

void UPrimitiveComponent::PutAllRigidBodiesToSleep()
{
	PutRigidBodyToSleep(NAME_None);
}


bool UPrimitiveComponent::RigidBodyIsAwake(FName BoneName)
{
	FBodyInstance* BI = GetBodyInstance(BoneName);
	if(BI)
	{
		return BI->IsInstanceAwake();
	}

	return false;
}

bool UPrimitiveComponent::IsAnyRigidBodyAwake()
{
	return RigidBodyIsAwake(NAME_None);
}


void UPrimitiveComponent::SetNotifyRigidBodyCollision(bool bNewNotifyRigidBodyCollision)
{
	BodyInstance.SetInstanceNotifyRBCollision(bNewNotifyRigidBodyCollision);
	OnComponentCollisionSettingsChanged();
}



void UPrimitiveComponent::SetPhysMaterialOverride(UPhysicalMaterial* NewPhysMaterial)
{
	BodyInstance.SetPhysMaterialOverride(NewPhysMaterial);
}

void UPrimitiveComponent::SyncComponentToRBPhysics()
{
	if(!IsRegistered())
	{
		UE_LOG(LogPhysics, Log, TEXT("SyncComponentToRBPhysics : Component not registered (%s)"), *GetPathName());
		return;
	}

	 // BodyInstance we are going to sync the component to
	FBodyInstance* UseBI = GetBodyInstance();
	if(UseBI == NULL || !UseBI->IsValidBodyInstance())
	{
		UE_LOG(LogPhysics, Log, TEXT("SyncComponentToRBPhysics : Missing or invalid BodyInstance (%s)"), *GetPathName());
		return;
	}

	AActor* Owner = GetOwner();
	if(Owner != NULL)
	{
		const bool bStillInWorld = Owner->CheckStillInWorld();
		if (!bStillInWorld || Owner->IsPendingKill() || IsPendingKill() || !IsSimulatingPhysics())
		{
			return;
		}
	}

	// See if the transform is actually different, and if so, move the component to match physics
	const FTransform NewTransform = UseBI->GetUnrealWorldTransform();	
	if(!NewTransform.Equals(ComponentToWorld))
	{
		const FVector MoveBy = NewTransform.GetLocation() - ComponentToWorld.GetLocation();
		const FRotator NewRotation = NewTransform.Rotator();

		//@warning: do not reference BodyInstance again after calling MoveComponent() - events from the move could have made it unusable (destroying the actor, SetPhysics(), etc)
		MoveComponent(MoveBy, NewRotation, false, NULL, MOVECOMP_SkipPhysicsMove);
	}
}

FBodyInstance* UPrimitiveComponent::GetBodyInstance(FName BoneName) const
{
	return const_cast<FBodyInstance*>(&BodyInstance);
}


float UPrimitiveComponent::GetDistanceToCollision(const FVector & Point, FVector& ClosestPointOnCollision) const
{
	ClosestPointOnCollision=Point;

	FBodyInstance* BodyInst = GetBodyInstance();
	if(BodyInst != NULL)
	{
		return BodyInst->GetDistanceToBody(Point, ClosestPointOnCollision);
	}

	return -1.f;
}

bool UPrimitiveComponent::IsSimulatingPhysics(FName BoneName) const
{
	FBodyInstance* BodyInst = GetBodyInstance(BoneName);
	if(BodyInst != NULL)
	{
		return BodyInst->IsInstanceSimulatingPhysics();
	}
	else
	{
		return false;
	}
}

FVector UPrimitiveComponent::GetComponentVelocity() const
{
	if (IsSimulatingPhysics())
	{
		FBodyInstance* BodyInst = GetBodyInstance();
		if(BodyInst != NULL)
		{
			return BodyInst->GetUnrealWorldVelocity();
		}
	}

	return Super::GetComponentVelocity();
}

void UPrimitiveComponent::SetCollisionObjectType(ECollisionChannel Channel)
{
	BodyInstance.SetObjectType(Channel);
}

void UPrimitiveComponent::SetCollisionResponseToChannel(ECollisionChannel Channel, ECollisionResponse NewResponse)
{
	BodyInstance.SetResponseToChannel(Channel, NewResponse);
	OnComponentCollisionSettingsChanged();
}

void UPrimitiveComponent::SetCollisionResponseToAllChannels(enum ECollisionResponse NewResponse)
{
	BodyInstance.SetResponseToAllChannels(NewResponse);
	OnComponentCollisionSettingsChanged();
}

void UPrimitiveComponent::SetCollisionResponseToChannels(const FCollisionResponseContainer& NewReponses)
{
	BodyInstance.SetResponseToChannels(NewReponses);
	OnComponentCollisionSettingsChanged();
}

void UPrimitiveComponent::SetCollisionEnabled(ECollisionEnabled::Type NewType)
{
	const bool bOldNavRelevant = IsRegistered() && IsNavigationRelevant(true);

	if (BodyInstance.GetCollisionEnabled() != NewType)
	{
		BodyInstance.SetCollisionEnabled(NewType);
		OnComponentCollisionSettingsChanged();

		EnsurePhysicsStateCreated();
	}

	AActor* Owner = GetOwner();
	if (Owner && IsRegistered())
	{
		const bool bNewNavRelevant = IsNavigationRelevant(true);
		const bool bOwnerNavRelevant = Owner->IsNavigationRelevant();

		if (bNewNavRelevant != bOldNavRelevant || bNewNavRelevant != bOwnerNavRelevant)
		{
			Owner->UpdateNavigationRelevancy();
		}
	}
}

// @todo : implement skeletalmeshcomponent version
void UPrimitiveComponent::SetCollisionProfileName(FName InCollisionProfileName)
{
	const bool bOldNavRelevant = IsRegistered() && IsNavigationRelevant(true);

	ECollisionEnabled::Type OldCollisionEnabled = BodyInstance.GetCollisionEnabled();
	BodyInstance.SetCollisionProfileName(InCollisionProfileName);
	OnComponentCollisionSettingsChanged();

	ECollisionEnabled::Type NewCollisionEnabled = BodyInstance.GetCollisionEnabled();

	if (OldCollisionEnabled != NewCollisionEnabled)
	{
		EnsurePhysicsStateCreated();
	}

	AActor* Owner = GetOwner();
	if (Owner && IsRegistered())
	{
		const bool bNewNavRelevant = IsNavigationRelevant(true);
		const bool bOwnerNavRelevant = Owner->IsNavigationRelevant();

		if (bNewNavRelevant != bOldNavRelevant || bNewNavRelevant != bOwnerNavRelevant)
		{
			Owner->UpdateNavigationRelevancy();
		}
	}
}

FName UPrimitiveComponent::GetCollisionProfileName()
{
	return BodyInstance.GetCollisionProfileName();
}

void UPrimitiveComponent::OnActorEnableCollisionChanged()
{
	BodyInstance.UpdatePhysicsFilterData();
	OnComponentCollisionSettingsChanged();
}

void UPrimitiveComponent::OnComponentCollisionSettingsChanged()
{
	if (!IsTemplate() && IsRegistered())			// not for CDOs
	{
		// changing collision settings could affect touching status, need to update
		UpdateOverlaps();
	}
}

bool UPrimitiveComponent::K2_LineTraceComponent(FVector TraceStart, FVector TraceEnd, bool bTraceComplex, bool bShowTrace, FVector& HitLocation, FVector& HitNormal)
{
	FHitResult Hit;
	static FName KismetTraceComponentName(TEXT("KismetTraceComponent"));
	FCollisionQueryParams LineParams(KismetTraceComponentName, bTraceComplex);
	const bool bDidHit = LineTraceComponent(Hit, TraceStart, TraceEnd, LineParams);

	if( bDidHit )
	{
		// Fill in the results if we hit
		HitLocation = Hit.Location;
		HitNormal = Hit.Normal;
	}
	else
	{
		// Blank these out to avoid confusion!
		HitLocation = FVector::ZeroVector;
		HitNormal = FVector::ZeroVector;
	}

	if( bShowTrace )
	{
		GetWorld()->LineBatcher->DrawLine(TraceStart, bDidHit ? HitLocation : TraceEnd, FLinearColor(1.f,0.5f,0.f), SDPG_World, 2.f);
		if( bDidHit )
		{
			GetWorld()->LineBatcher->DrawLine(HitLocation, TraceEnd, FLinearColor(0.f,0.5f,1.f), SDPG_World, 2.f);
		}
	}

	return bDidHit;
}


ECollisionEnabled::Type UPrimitiveComponent::GetCollisionEnabled() const
{
	return BodyInstance.GetCollisionEnabled();
}

ECollisionResponse UPrimitiveComponent::GetCollisionResponseToChannel(ECollisionChannel Channel) const
{
	return BodyInstance.GetResponseToChannel(Channel);
}

const FCollisionResponseContainer & UPrimitiveComponent::GetCollisionResponseToChannels() const
{
	return BodyInstance.GetResponseToChannels();
}

void UPrimitiveComponent::UpdatePhysicsToRBChannels()
{
	if(BodyInstance.IsValidBodyInstance())
	{
		BodyInstance.UpdatePhysicsFilterData();
	}
}

