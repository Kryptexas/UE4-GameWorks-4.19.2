// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

//----------------------------------------------------------------------//
// FNavAgentProperties
//----------------------------------------------------------------------//
void FNavAgentProperties::UpdateWithCollisionComponent(UShapeComponent* CollisionComponent)
{
	check( CollisionComponent != NULL);
	AgentRadius = CollisionComponent->Bounds.SphereRadius;
}

//----------------------------------------------------------------------//
// UMovementComponent
//----------------------------------------------------------------------//
UNavMovementComponent::UNavMovementComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FBasedPosition UNavMovementComponent::GetActorLocationBased() const
{
	return FBasedPosition(NULL, GetActorLocation());
}

void UNavMovementComponent::RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed)
{
	Velocity = MoveVelocity;
}

bool UNavMovementComponent::CanStopPathFollowing() const
{
	return true;
}
