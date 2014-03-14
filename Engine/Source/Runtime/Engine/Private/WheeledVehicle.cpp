// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Vehicle.cpp: AWheeledVehicle implementation
	TODO: Put description here
=============================================================================*/

#include "EnginePrivate.h"

FName AWheeledVehicle::VehicleMovementComponentName(TEXT("MovementComp"));
FName AWheeledVehicle::VehicleMeshComponentName(TEXT("VehicleMesh"));

AWheeledVehicle::AWheeledVehicle(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	Mesh = PCIP.CreateDefaultSubobject<USkeletalMeshComponent>(this, VehicleMeshComponentName);
	static FName CollisionProfileName(TEXT("Vehicle"));
	Mesh->SetCollisionProfileName(CollisionProfileName);
	Mesh->BodyInstance.bSimulatePhysics = true;
	Mesh->BodyInstance.bNotifyRigidBodyCollision = true;
	Mesh->BodyInstance.bUseCCD = true;
	Mesh->bBlendPhysics = true;
	Mesh->bUseSingleBodyPhysics = true;
	Mesh->bGenerateOverlapEvents = true;
	RootComponent = Mesh;

	VehicleMovement = PCIP.CreateDefaultSubobject<UWheeledVehicleMovementComponent, UWheeledVehicleMovementComponent4W>(this, VehicleMovementComponentName);
	VehicleMovement->SetIsReplicated(true); // Enable replication by default
	VehicleMovement->UpdatedComponent = Mesh;
}

