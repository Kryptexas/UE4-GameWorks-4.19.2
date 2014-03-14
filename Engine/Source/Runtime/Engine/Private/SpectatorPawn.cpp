// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "EnginePrivate.h"


ASpectatorPawn::ASpectatorPawn(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP
	.SetDefaultSubobjectClass<USpectatorPawnMovement>(Super::MovementComponentName)
	.DoNotCreateDefaultSubobject(Super::MeshComponentName)
	)
{
	bCanBeDamaged = false;
	SetRemoteRoleForBackwardsCompat(ROLE_None);
	bReplicates = false;

	BaseEyeHeight = 0.0f;
	bCollideWhenPlacing = false;

	static FName CollisionProfileName(TEXT("Spectator"));
	CollisionComponent->SetCollisionProfileName(CollisionProfileName);
}


void ASpectatorPawn::PossessedBy(class AController* NewController)
{
	Controller = NewController;
}

