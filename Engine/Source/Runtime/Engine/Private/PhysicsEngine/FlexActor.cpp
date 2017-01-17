// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

#include "PhysicsEngine/FlexActor.h"
#include "PhysicsEngine/FlexComponent.h"


AFlexActor::AFlexActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UFlexComponent>(TEXT("StaticMeshComponent0")))
{
	UFlexComponent* FlexMeshComponent = CastChecked<UFlexComponent>(StaticMeshComponent);
	RootComponent = FlexMeshComponent;
	FlexMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	FlexMeshComponent->Mobility = EComponentMobility::Movable;
	FlexMeshComponent->SetSimulatePhysics(false);

	PrimaryActorTick.bCanEverTick = false;
}