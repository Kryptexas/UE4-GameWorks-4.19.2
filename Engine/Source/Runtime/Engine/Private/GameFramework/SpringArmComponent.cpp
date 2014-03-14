// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

//////////////////////////////////////////////////////////////////////////
// USpringArmComponent

const FName USpringArmComponent::SocketName(TEXT("SpringEndpoint"));

USpringArmComponent::USpringArmComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PrimaryComponentTick.bCanEverTick = true;
	
	bAutoActivate = true;
	bTickInEditor = true;
	bUseControllerViewRotation = false;
	bDoCollisionTest = true;

	TargetArmLength = 300.0f;
	ProbeSize = 12.0f;
	ProbeChannel = ECC_Camera;
}

void USpringArmComponent::UpdateDesiredArmLocation(const FVector& Origin, const FRotator& Direction, bool bDoTrace, float DeltaTime)
{
	FRotator DesiredRot = Direction;

	FVector DesiredLoc = Origin;
	DesiredLoc -= DesiredRot.Vector() * TargetArmLength;
	DesiredLoc += FRotationMatrix(DesiredRot).TransformVector(SocketOffset);

	FVector ResultLoc;
	if (bDoTrace && (TargetArmLength != 0.0f))
	{
		static FName TraceTagName(TEXT("SpringArm"));
		FCollisionQueryParams QueryParams(TraceTagName, false, GetOwner());

		FHitResult Result;
		GetWorld()->SweepSingle(Result, Origin, DesiredLoc, FQuat::Identity, ProbeChannel, FCollisionShape::MakeSphere(ProbeSize), QueryParams);

		ResultLoc = BlendLocations(DesiredLoc, Result.Location, Result.bBlockingHit, DeltaTime);
	}
	else
	{
		ResultLoc = DesiredLoc;
	}

	RelativeSocketLocation = ComponentToWorld.ToInverseMatrixWithScale().TransformPosition(ResultLoc);
	UpdateChildTransforms();
}

FVector USpringArmComponent::BlendLocations(const FVector& DesiredArmLocation, const FVector& TraceHitLocation, bool bHitSomething, float DeltaTime)
{
	return bHitSomething ? TraceHitLocation : DesiredArmLocation;
}

void USpringArmComponent::OnRegister()
{
	Super::OnRegister();
	UpdateDesiredArmLocation(GetComponentLocation(), GetComponentRotation(), false, 0.0f);
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

	UpdateDesiredArmLocation(GetComponentLocation(), GetComponentRotation(), bDoCollisionTest, DeltaTime);
}

FTransform USpringArmComponent::GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace) const
{
	const FQuat RelativeSocketRotation(FRotator(0.0f, 0.0f, 0.0f));
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
