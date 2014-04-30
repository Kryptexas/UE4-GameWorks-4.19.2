// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Vehicle.cpp: AWheeledVehicle implementation
	TODO: Put description here
=============================================================================*/

#include "EnginePrivate.h"
#include "DisplayDebugHelpers.h"

FName AWheeledVehicle::VehicleMovementComponentName(TEXT("MovementComp"));
FName AWheeledVehicle::VehicleMeshComponentName(TEXT("VehicleMesh"));

AWheeledVehicle::AWheeledVehicle(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	Mesh = PCIP.CreateDefaultSubobject<USkeletalMeshComponent>(this, VehicleMeshComponentName);
	Mesh->SetCollisionProfileName(UCollisionProfile::Vehicle_ProfileName);
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

void AWheeledVehicle::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	static FName NAME_Vehicle = FName(TEXT("Vehicle"));

	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);

#if WITH_PHYSX
	if (DebugDisplay.IsDisplayOn(NAME_Vehicle))
	{
		GetVehicleMovementComponent()->DrawDebug(Canvas, YL, YPos);
	}
#endif
}