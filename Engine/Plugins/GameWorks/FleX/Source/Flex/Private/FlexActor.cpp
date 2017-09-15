// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "FlexActor.h"
#include "FlexComponent.h"
#include "Engine/CollisionProfile.h"

AFlexActor::AFlexActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UFlexComponent>(TEXT("StaticMeshComponent0")))
{
	UFlexComponent* FlexMeshComponent = CastChecked<UFlexComponent>(GetStaticMeshComponent());
	RootComponent = FlexMeshComponent;
	FlexMeshComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	FlexMeshComponent->Mobility = EComponentMobility::Movable;
	FlexMeshComponent->SetSimulatePhysics(false);

	PrimaryActorTick.bCanEverTick = false;
}