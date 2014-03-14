// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"


URotatingMovementComponent::URotatingMovementComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	RotationRate.Yaw = 180.0f;
	bRotationInLocalSpace = true;
}


void URotatingMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	// skip if don't want component updated when not rendered
	if ( SkipUpdate(DeltaTime) )
	{
		return;
	}

	// Compute new rotation
	const FQuat OldRotation = UpdatedComponent->GetComponentQuat();
	const FQuat DeltaRotation = (RotationRate * DeltaTime).Quaternion();
	const FQuat NewRotation = bRotationInLocalSpace ? (OldRotation * DeltaRotation) : (DeltaRotation * OldRotation);

	// Compute new location
	FVector NewLocation = UpdatedComponent->GetComponentLocation();
	if (!PivotTranslation.IsZero())
	{
		const FVector OldPivot = OldRotation.RotateVector(PivotTranslation);
		const FVector NewPivot = NewRotation.RotateVector(PivotTranslation);
		NewLocation = (NewLocation + OldPivot - NewPivot);
	}

	UpdatedComponent->SetWorldLocationAndRotation(NewLocation, NewRotation);
}
