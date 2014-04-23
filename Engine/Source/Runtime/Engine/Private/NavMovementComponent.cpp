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
	, bUpdateNavAgentWithOwnersCollision(true)
{
}

FBasedPosition UNavMovementComponent::GetActorFeetLocationBased() const
{
	return FBasedPosition(NULL, GetActorFeetLocation());
}

void UNavMovementComponent::RequestDirectMove(const FVector& MoveVelocity, bool bForceMaxSpeed)
{
	Velocity = MoveVelocity;
}

bool UNavMovementComponent::CanStopPathFollowing() const
{
	return true;
}

void UNavMovementComponent::UpdateNavAgent(AActor* Owner)
{
	ensure(Owner == NULL || Owner == GetOwner());
	if (Owner == NULL || ShouldUpdateNavAgentWithOwnersCollision() == false)
	{
		return;
	}

	// Can't call GetSimpleCollisionCylinder(), because no components will be registered.
	float BoundRadius, BoundHalfHeight;	
	Owner->GetSimpleCollisionCylinder(BoundRadius, BoundHalfHeight);
	NavAgentProps.AgentRadius = BoundRadius;
	NavAgentProps.AgentHeight = BoundHalfHeight * 2.f;
}

void UNavMovementComponent::UpdateNavAgent(class UCapsuleComponent* CapsuleComponent)
{
	if (CapsuleComponent == NULL || ShouldUpdateNavAgentWithOwnersCollision() == false)
	{
		return;
	}

	NavAgentProps.AgentRadius = CapsuleComponent->GetScaledCapsuleRadius();
	NavAgentProps.AgentHeight = CapsuleComponent->GetScaledCapsuleHalfHeight() * 2.f;
}