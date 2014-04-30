// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

//////////////////////////////////////////////////////////////////////////
// USpringArmComponent

const FName USpringArmComponent::SocketName(TEXT("SpringEndpoint"));

extern float GAverageMS;

USpringArmComponent::USpringArmComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PrimaryComponentTick.bCanEverTick = true;
	
	bAutoActivate = true;
	bTickInEditor = true;
	bUseControllerViewRotation = false;
	bDoCollisionTest = true;

	bInheritPitch = true;
	bInheritYaw = true;
	bInheritRoll = true;

	TargetArmLength = 300.0f;
	ProbeSize = 12.0f;
	ProbeChannel = ECC_Camera;

	CameraLagSpeed = 10.f;
	CameraRotationLagSpeed = 10.f;

	RelativeSocketRotation = FQuat::Identity;
}

void USpringArmComponent::UpdateDesiredArmLocation(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime)
{
	FRotator DesiredRot = GetComponentRotation();

	// If inheriting rotation, check options for which components to inherit
	if(!bAbsoluteRotation)
	{
		if(!bInheritPitch)
		{
			DesiredRot.Pitch = RelativeRotation.Pitch;
		}

		if (!bInheritYaw)
		{
			DesiredRot.Yaw = RelativeRotation.Yaw;
		}

		if (!bInheritRoll)
		{
			DesiredRot.Roll = RelativeRotation.Roll;
		}
	}

	// Apply 'lag' to rotation if desired
	if(bDoRotationLag)
	{
		DesiredRot = FMath::RInterpTo(PreviousDesiredRot, DesiredRot, GAverageMS/1000.f, CameraRotationLagSpeed);
	}
	PreviousDesiredRot = DesiredRot;

	// Get the spring arm 'origin', the target we want to look at
	FVector ArmOrigin = GetComponentLocation() + TargetOffset;
	// We lag the target, not the actuall camera position, so rotating the camera around does not hav lag
	FVector DesiredLoc = ArmOrigin;
	if (bDoLocationLag)
	{
		DesiredLoc = FMath::VInterpTo(PreviousDesiredLoc, DesiredLoc, GAverageMS/1000.f, CameraLagSpeed);
	}
	PreviousDesiredLoc = DesiredLoc;

	// Now offset camera position back along our rotation
	DesiredLoc -= DesiredRot.Vector() * TargetArmLength;
	// Add socket offset in local space
	DesiredLoc += FRotationMatrix(DesiredRot).TransformVector(SocketOffset);

	// Do a sweep to ensure we are not penetrating the world
	FVector ResultLoc;
	if (bDoTrace && (TargetArmLength != 0.0f))
	{
		static FName TraceTagName(TEXT("SpringArm"));
		FCollisionQueryParams QueryParams(TraceTagName, false, GetOwner());

		FHitResult Result;
		GetWorld()->SweepSingle(Result, ArmOrigin, DesiredLoc, FQuat::Identity, ProbeChannel, FCollisionShape::MakeSphere(ProbeSize), QueryParams);

		ResultLoc = BlendLocations(DesiredLoc, Result.Location, Result.bBlockingHit, DeltaTime);
	}
	else
	{
		ResultLoc = DesiredLoc;
	}

	// Form a transform for new world transform for camera
	FTransform WorldCamTM(DesiredRot, ResultLoc);
	// Convert to relative to component
	FTransform RelCamTM = WorldCamTM.GetRelativeTransform(ComponentToWorld);

	// Update socket location/rotation
	RelativeSocketLocation = RelCamTM.GetLocation();
	RelativeSocketRotation = RelCamTM.GetRotation();

	UpdateChildTransforms();
}

FVector USpringArmComponent::BlendLocations(const FVector& DesiredArmLocation, const FVector& TraceHitLocation, bool bHitSomething, float DeltaTime)
{
	return bHitSomething ? TraceHitLocation : DesiredArmLocation;
}

void USpringArmComponent::OnRegister()
{
	Super::OnRegister();
	UpdateDesiredArmLocation(false, false, false, 0.f);
}

void USpringArmComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bUseControllerViewRotation)
	{
		if (APawn* OwningPawn = Cast<APawn>(GetOwner()))
		{
			const FRotator PawnViewRotation = OwningPawn->GetViewRotation();
			if (PawnViewRotation != GetComponentRotation())
			{
				SetWorldRotation(PawnViewRotation);
			}
		}
	}

	UpdateDesiredArmLocation(bDoCollisionTest, bEnableCameraLag, bEnableCameraRotationLag, DeltaTime);
}

FTransform USpringArmComponent::GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace) const
{
	FTransform RelativeTransform(RelativeSocketRotation, RelativeSocketLocation);

	switch(TransformSpace)
	{
		case RTS_World:
		{
			return RelativeTransform * ComponentToWorld;
			break;
		}
		case RTS_Actor:
		{
			if( const AActor* Actor = GetOwner() )
			{
				FTransform SocketTransform = RelativeTransform * ComponentToWorld;
				return SocketTransform.GetRelativeTransform(Actor->GetTransform());
			}
			break;
		}
		case RTS_Component:
		{
			return RelativeTransform;
		}
	}
	return RelativeTransform;
}

bool USpringArmComponent::HasAnySockets() const
{
	return true;
}

void USpringArmComponent::QuerySupportedSockets(TArray<FComponentSocketDescription>& OutSockets) const
{
	new (OutSockets) FComponentSocketDescription(SocketName, EComponentSocketType::Socket);
}
