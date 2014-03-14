// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

#if WITH_PHYSX
#include "../PhysicsEngine/PhysXSupport.h"
#include "../Vehicles/PhysXVehicleManager.h"
#endif // WITH_PHYSX


UVehicleWheel::UVehicleWheel(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CollisionMeshObj(TEXT("/Engine/EngineMeshes/Cylinder"));
	CollisionMesh = CollisionMeshObj.Object;

	ShapeRadius = 30.0f;
	ShapeWidth = 10.0f;
	bAutoAdjustCollisionSize = true;
	//Mass = 10.0f;
	Inertia = 1.0f;
	bAffectedByHandbrake = true;
	//DampingRate = 100.0f;
	LatStiffMaxLoad = 2.0f;
	LatStiffValue = 17.0f;
	LatStiffFactor = 1.0f;
	LongStiffValue = 10.0f;
	SuspensionForceOffset = 0.0f;
	SuspensionMaxRaise = 20.0f;
	SuspensionMaxDrop = 20.0f;
	SuspensionNaturalFrequency = 5.0f;
	SuspensionDampingRatio = 1.0f;
}

float UVehicleWheel::GetSteerAngle()
{
#if WITH_PHYSX
	return FMath::RadiansToDegrees( VehicleSim->GetWorld()->GetPhysicsScene()->GetVehicleManager()->GetWheelsStates(VehicleSim)[WheelIndex].steerAngle );
#else
	return 0.0f;
#endif // WITH_PHYSX
}

float UVehicleWheel::GetRotationAngle()
{
#if WITH_PHYSX
	float RotationAngle = -1.0f * FMath::RadiansToDegrees( VehicleSim->PVehicle->mWheelsDynData.getWheelRotationAngle( WheelIndex ) );
	check(!FMath::IsNaN(RotationAngle));
	return RotationAngle;
#else
	return 0.0f;
#endif // WITH_PHYSX
}

float UVehicleWheel::GetSuspensionOffset()
{
#if WITH_PHYSX
	return VehicleSim->GetWorld()->GetPhysicsScene()->GetVehicleManager()->GetWheelsStates(VehicleSim)[WheelIndex].suspJounce;
#else
	return 0.0f;
#endif // WITH_PHYSX
}

#if WITH_PHYSX

void UVehicleWheel::Init( UWheeledVehicleMovementComponent* InVehicleSim, int32 InWheelIndex )
{
	check(InVehicleSim);
	check(InVehicleSim->Wheels.IsValidIndex(InWheelIndex));

	VehicleSim = InVehicleSim;
	WheelIndex = InWheelIndex;

	const int32 WheelShapeIdx = VehicleSim->PVehicle->mWheelsSimData.getWheelShapeMapping( WheelIndex );
	check(WheelShapeIdx >= 0);

	WheelShape = NULL;
	VehicleSim->PVehicle->getRigidDynamicActor()->getShapes( &WheelShape, 1, WheelShapeIdx );
	check(WheelShape);

	Location = GetPhysicsLocation();
	OldLocation = Location;
}

void UVehicleWheel::Shutdown()
{
	WheelShape = NULL;
}

FWheelSetup& UVehicleWheel::GetWheelSetup()
{
	return VehicleSim->WheelSetups[WheelIndex];
}

void UVehicleWheel::Tick( float DeltaTime )
{
	OldLocation = Location;
	Location = GetPhysicsLocation();
	Velocity = ( Location - OldLocation ) / DeltaTime;
}

FVector UVehicleWheel::GetPhysicsLocation()
{
	if ( WheelShape )
	{
		PxVec3 PLocation = VehicleSim->PVehicle->getRigidDynamicActor()->getGlobalPose().transform( WheelShape->getLocalPose() ).p;
		return P2UVector( PLocation );
	}

	return FVector(0.0f);
}

#if WITH_EDITOR

void UVehicleWheel::PostEditChangeProperty( FPropertyChangedEvent& PropertyChangedEvent )
{
	Super::PostEditChangeProperty( PropertyChangedEvent );

	// Trigger a runtime rebuild of the PhysX vehicle
	FPhysXVehicleManager::VehicleSetupTag++;
}

#endif //WITH_EDITOR

#endif // WITH_PHYSX

UPhysicalMaterial* UVehicleWheel::GetContactSurfaceMaterial()
{
	UPhysicalMaterial* PhysMaterial = NULL;

#if WITH_PHYSX
	const PxMaterial* ContactSurface = VehicleSim->GetWorld()->GetPhysicsScene()->GetVehicleManager()->GetWheelsStates(VehicleSim)[WheelIndex].tireSurfaceMaterial;
	if (ContactSurface)
	{
		PhysMaterial = FPhysxUserData::Get<UPhysicalMaterial>(ContactSurface->userData);
	}
#endif

	return PhysMaterial;
}
