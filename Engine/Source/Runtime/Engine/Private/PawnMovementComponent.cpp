// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

DEFINE_LOG_CATEGORY_STATIC(LogPawnMovementComponent, Log, All);


//----------------------------------------------------------------------//
// UPawnMovementComponent
//----------------------------------------------------------------------//
UPawnMovementComponent::UPawnMovementComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UPawnMovementComponent::SetUpdatedComponent(UPrimitiveComponent* NewUpdatedComponent)
{
	if (NewUpdatedComponent)
	{
		if (!ensureMsgf(Cast<APawn>(NewUpdatedComponent->GetOwner()), TEXT("%s must update a component owned by a Pawn"), *GetName()))
		{
			return;
		}
	}

	Super::SetUpdatedComponent(NewUpdatedComponent);

	PawnOwner = NewUpdatedComponent ? CastChecked<APawn>(NewUpdatedComponent->GetOwner()) : NULL;
}


bool UPawnMovementComponent::IsMoveInputIgnored() const
{
	if (UpdatedComponent)
	{
		if (PawnOwner)
		{
			const APlayerController* PCOwner = Cast<const APlayerController>(PawnOwner->Controller);
			if (PCOwner)
			{
				return PCOwner->IsMoveInputIgnored();
			}
			else
			{
				// Allow movement by non-player controllers, if for some reason AI wants to go through simulated input.
				return false;
			}
		}
	}

	// No UpdatedComponent or Pawn, no movement.
	return true;
}


void UPawnMovementComponent::AddInputVector(FVector WorldAccel, bool bForce /*=false*/)
{
	if (bForce || !IsMoveInputIgnored())
	{
		ControlInputVector += WorldAccel;
	}
}

FVector UPawnMovementComponent::ConsumeInputVector()
{
	const FVector OldValue = GetInputVector();
	ControlInputVector = FVector::ZeroVector;
	return OldValue;
}
