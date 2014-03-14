// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Net/UnrealNetwork.h"

#if WITH_PHYSX
#include "../PhysicsEngine/PhysXSupport.h"
#include "../Collision/PhysXCollision.h"
#include "PhysXVehicleManager.h"
#endif //WITH_PHYSX

// Temporary until curve editing in
void ConvertCurve( const TArray<FFloatPair>& Pairs, FRichCurve& OutCurve )
{
	OutCurve.Reset();

	for ( auto It = Pairs.CreateConstIterator(); It; ++It )
	{
		OutCurve.AddKey( It->InVal, It->OutVal );
	}
}

#if WITH_PHYSX

/**
 * PhysX shader for tire friction forces
 * tireFriction - friction value of the tire contact.
 * longSlip - longitudinal slip of the tire.
 * latSlip - lateral slip of the tire.
 * camber - camber angle of the tire
 * wheelOmega - rotational speed of the wheel.
 * wheelRadius - the distance from the tire surface and the center of the wheel.
 * recipWheelRadius - the reciprocal of wheelRadius.
 * restTireLoad - the load force experienced by the tire when the vehicle is at rest.
 * normalisedTireLoad - a value equal to the load force on the tire divided by the restTireLoad.
 * tireLoad - the load force currently experienced by the tire.
 * gravity - magnitude of gravitational acceleration.
 * recipGravity - the reciprocal of the magnitude of gravitational acceleration.
 * wheelTorque - the torque to be applied to the wheel around the wheel axle.
 * tireLongForceMag - the magnitude of the longitudinal tire force to be applied to the vehicle's rigid body.
 * tireLatForceMag - the magnitude of the lateral tire force to be applied to the vehicle's rigid body.
 * tireAlignMoment - the aligning moment of the tire that is to be applied to the vehicle's rigid body (not currently used). 
 */
void PTireShader(const void* shaderData, const PxF32 tireFriction,
	const PxF32 longSlip, const PxF32 latSlip,
	const PxF32 camber, const PxF32 wheelOmega, const PxF32 wheelRadius, const PxF32 recipWheelRadius,
	const PxF32 restTireLoad, const PxF32 normalisedTireLoad, const PxF32 tireLoad,
	const PxF32 gravity, const PxF32 recipGravity,
	PxF32& wheelTorque, PxF32& tireLongForceMag, PxF32& tireLatForceMag, PxF32& tireAlignMoment)
{
	UVehicleWheel* Wheel = (UVehicleWheel*)shaderData;

	FTireShaderInput Input;

	Input.TireFriction = tireFriction;
	Input.LongSlip = longSlip;
	Input.LatSlip = latSlip;
	Input.WheelOmega = wheelOmega;
	Input.WheelRadius = wheelRadius;
	Input.RecipWheelRadius = recipWheelRadius;
	Input.NormalizedTireLoad = FMath::Clamp( normalisedTireLoad, Wheel->VehicleSim->MinNormalizedTireLoad, Wheel->VehicleSim->MaxNormalizedTireLoad );
	Input.RestTireLoad = restTireLoad;
	Input.TireLoad = restTireLoad * Input.NormalizedTireLoad;
	Input.Gravity = gravity;
	Input.RecipGravity = recipGravity;

	FTireShaderOutput Output(0.0f);

	Wheel->VehicleSim->GenerateTireForces( Wheel, Input, Output );

	wheelTorque = Output.WheelTorque;
	tireLongForceMag = Output.LongForce;
	tireLatForceMag = Output.LatForce;
	
	if ( /*Wheel->bDebugWheels*/true )
	{
		Wheel->DebugLongSlip = longSlip;
		Wheel->DebugLatSlip = latSlip;
		Wheel->DebugNormalizedTireLoad = normalisedTireLoad;
		Wheel->DebugWheelTorque = wheelTorque;
		Wheel->DebugLongForce = tireLongForceMag;
		Wheel->DebugLatForce = tireLatForceMag;
	}
}

#endif // WITH_PHYSX

UWheeledVehicleMovementComponent::UWheeledVehicleMovementComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	Mass = 1500.0f;
	InertiaTensorScale = FVector( 1.0f, 0.80f, 1.0f );
	AngErrorAccumulator = 0.0f;
	MinNormalizedTireLoad = 0.0f;
	MaxNormalizedTireLoad = 10.0f;
	
	IdleBrakeInput = 0.05f;
	StopThreshold = 10.0f; 
	ThrottleInputRate.RiseRate = 6.0f;
	ThrottleInputRate.FallRate = 10.0f;
	BrakeInputRate.RiseRate = 6.0f;
	BrakeInputRate.FallRate = 10.0f;
	HandbrakeInputRate.RiseRate = 12.0f;
	HandbrakeInputRate.FallRate = 12.0f;
	SteeringInputRate.RiseRate = 2.5f;
	SteeringInputRate.FallRate = 5.0f;
}

#if WITH_PHYSX

bool UWheeledVehicleMovementComponent::CanCreateVehicle()
{
	if ( UpdatedComponent == NULL )
	{
		UE_LOG( LogVehicles, Warning, TEXT("Cannot create vehicle for %s. UpdatedComponent is not set."),
			*GetPathName() );
		return false;
	}

	if ( UpdatedComponent->BodyInstance.GetPxRigidDynamic() == NULL )
	{
		UE_LOG( LogVehicles, Warning, TEXT("Cannot create vehicle for %s. UpdatedComponent has not initialized its rigid body actor."),
			*GetPathName() );
		return false;
	}

	for ( int32 WheelIdx = 0; WheelIdx < WheelSetups.Num(); ++WheelIdx )
	{
		if ( WheelSetups[WheelIdx].WheelClass == NULL )
		{
			UE_LOG( LogVehicles, Warning, TEXT("Cannot create vehicle for %s. Wheel %d is not set."),
				*GetPathName(), WheelIdx );
			return false;
		}
	}

	return true;
}

void UWheeledVehicleMovementComponent::CreateVehicle()
{
	if ( PVehicle == NULL )
	{
		if ( CanCreateVehicle() )
		{
			check(UpdatedComponent);
			check(UpdatedComponent->BodyInstance.GetPxRigidDynamic());

			SetupVehicle();

			if ( PVehicle != NULL )
			{
				PostSetupVehicle();
			}
		}
	}
}

void UWheeledVehicleMovementComponent::SetupVehicleShapes()
{
	PxRigidDynamic* PVehicleActor = UpdatedComponent->BodyInstance.GetPxRigidDynamic();

	static PxMaterial* WheelMaterial = GPhysXSDK->createMaterial(0.0f, 0.0f, 0.0f);

	// Add wheel shapes to actor
	for ( int32 WheelIdx = 0; WheelIdx < WheelSetups.Num(); ++WheelIdx )
	{
		FWheelSetup& WheelSetup = WheelSetups[WheelIdx];
		UVehicleWheel* Wheel = WheelSetup.WheelClass.GetDefaultObject();
		check(Wheel);

		const FVector WheelOffset = GetWheelRestingPosition( WheelSetup );
		const PxTransform PLocalPose = PxTransform( U2PVector( WheelOffset ) );
		PxShape* PWheelShape = NULL;

		// Prepare shape
		if (Wheel->bDontCreateShape)
		{
			continue;
		}
		else if (Wheel->CollisionMesh && Wheel->CollisionMesh->BodySetup)
		{
			FBoxSphereBounds MeshBounds = Wheel->CollisionMesh->GetBounds();
			FVector MeshScaleV = FVector(1,1,1);
			if (Wheel->bAutoAdjustCollisionSize)
			{
				MeshScaleV.X = Wheel->ShapeRadius / MeshBounds.BoxExtent.X;
				MeshScaleV.Y = Wheel->ShapeWidth / MeshBounds.BoxExtent.Y;
				MeshScaleV.Z = Wheel->ShapeRadius / MeshBounds.BoxExtent.Z;
			}
			PxMeshScale MeshScale( U2PVector(UpdatedComponent->RelativeScale3D * MeshScaleV), PxQuat::createIdentity() );
			if (Wheel->CollisionMesh->BodySetup->TriMesh)
			{
				PxTriangleMesh* TriMesh = Wheel->CollisionMesh->BodySetup->TriMesh;

				// No eSIMULATION_SHAPE flag for wheels
				PWheelShape = PVehicleActor->createShape( PxTriangleMeshGeometry(TriMesh, MeshScale), *WheelMaterial, PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eVISUALIZATION );
				PWheelShape->setLocalPose(PLocalPose);			
			}
			else if (Wheel->CollisionMesh->BodySetup->AggGeom.ConvexElems.Num() == 1)
			{
				PxConvexMesh* ConvexMesh = Wheel->CollisionMesh->BodySetup->AggGeom.ConvexElems[0].ConvexMesh;
				PWheelShape = PVehicleActor->createShape( PxConvexMeshGeometry(ConvexMesh, MeshScale), *WheelMaterial, PLocalPose );
			}

			check(PWheelShape);
		}
		else
		{
			PWheelShape = PVehicleActor->createShape( PxSphereGeometry(Wheel->ShapeRadius), *WheelMaterial, PLocalPose );
		}

		// Init filter data
		FCollisionResponseContainer CollisionResponse;
		CollisionResponse.SetAllChannels( ECR_Ignore );

		PxFilterData PWheelQueryFilterData, PDummySimData;
		CreateShapeFilterData( ECC_Vehicle, GetOwner()->GetUniqueID(), CollisionResponse, 0, 0, PWheelQueryFilterData, PDummySimData, false, false, false );

		//// Give suspension raycasts the same group ID as the chassis so that they don't hit each other
		PWheelShape->setQueryFilterData( PWheelQueryFilterData );
	}
}

void UWheeledVehicleMovementComponent::SetupVehicleMass()
{
	const FBodyInstance& BodyInst = UpdatedComponent->BodyInstance;
	PxRigidDynamic* PVehicleActor = BodyInst.GetPxRigidDynamic();

	// Override mass
	const float MassRatio = Mass > 0.0f ? Mass / PVehicleActor->getMass() : 1.0f;

	PxVec3 PInertiaTensor = PVehicleActor->getMassSpaceInertiaTensor();

	PInertiaTensor.x *= InertiaTensorScale.X * MassRatio;
	PInertiaTensor.y *= InertiaTensorScale.Y * MassRatio;
	PInertiaTensor.z *= InertiaTensorScale.Z * MassRatio;

	PVehicleActor->setMassSpaceInertiaTensor( PInertiaTensor );
	PVehicleActor->setMass( Mass );

	// Adding shapes fudges with the local pose, fix it now
	const PxVec3 PCOMOffset = U2PVector( GetCOMOffset() );
	PVehicleActor->setCMassLocalPose( PxTransform( PCOMOffset, PxQuat::createIdentity() ) );
}

void UWheeledVehicleMovementComponent::SetupWheels( PxVehicleWheelsSimData* PWheelsSimData )
{
	const FBodyInstance& VehicleBodyInstance = UpdatedComponent->BodyInstance;
	PxRigidDynamic* PVehicleActor = VehicleBodyInstance.GetPxRigidDynamic();

	const PxReal LengthScale = 100.f; // Convert default from m to cm

	// Control substepping
	PWheelsSimData->setSubStepCount( 5.f * LengthScale, 5, 2 ); // forcing constant substeps until all the kinks are ironed out	
	PWheelsSimData->setMinLongSlipDenominator(4.f * LengthScale * LengthScale); 

	// Prealloc data for the sprung masses
	PxVec3 WheelOffsets[32];
	float SprungMasses[32];
	check(WheelSetups.Num() <= 32);

	// Calculate wheel offsets first, necessary for sprung masses
	for ( int32 WheelIdx = 0; WheelIdx < WheelSetups.Num(); ++WheelIdx )
	{
		WheelOffsets[WheelIdx] = U2PVector( GetWheelRestingPosition( WheelSetups[WheelIdx] ) );
	}

	// Now that we have all the wheel offsets, calculate the sprung masses
	PxVec3 PCOMOffset = U2PVector(GetCOMOffset());
	PxVehicleComputeSprungMasses( WheelSetups.Num(), WheelOffsets, PCOMOffset, PVehicleActor->getMass(), 2, SprungMasses );

	for ( int32 WheelIdx = 0; WheelIdx < WheelSetups.Num(); ++WheelIdx )
	{
		UVehicleWheel* Wheel = WheelSetups[WheelIdx].WheelClass.GetDefaultObject();

		// init wheel data
		PxVehicleWheelData PWheelData;
		PWheelData.mRadius = Wheel->ShapeRadius;
		PWheelData.mWidth = Wheel->ShapeWidth;	
		PWheelData.mMaxSteer = FMath::DegreesToRadians(WheelSetups[WheelIdx].SteerAngle);
		PWheelData.mMaxBrakeTorque = WheelSetups[WheelIdx].MaxBrakeTorque;
		PWheelData.mMaxHandBrakeTorque = Wheel->bAffectedByHandbrake ? WheelSetups[WheelIdx].MaxHandBrakeTorque : 0.0f;

//		PWheelData.mMass = 1.0f;
//		PWheelData.mMOI = Wheel->Inertia * Mass; // Multiply by vehicle mass so that as vehicle mass shrinks, the torque and inertia can shrink as well
		PWheelData.mDampingRate = WheelSetups[WheelIdx].DampingRate;
		PWheelData.mMass = WheelSetups[WheelIdx].Mass;
		PWheelData.mMOI = 0.5f * PWheelData.mMass * FMath::Square(PWheelData.mRadius);
		
		// init tire data
		PxVehicleTireData PTireData; 
		PTireData.mType = Wheel->TireType ? Wheel->TireType->GetTireTypeID() : GEngine->DefaultTireType->GetTireTypeID();
		PTireData.mCamberStiffnessPerUnitGravity = 0.0f;
		PTireData.mLatStiffX = Wheel->LatStiffMaxLoad;
		PTireData.mLatStiffY = Wheel->LatStiffValue;
		PTireData.mLongitudinalStiffnessPerUnitGravity = Wheel->LongStiffValue;

		// init suspension data
		PxVehicleSuspensionData PSuspensionData;
		PSuspensionData.mSprungMass = SprungMasses[WheelIdx];
		PSuspensionData.mMaxCompression = Wheel->SuspensionMaxRaise;
		PSuspensionData.mMaxDroop = Wheel->SuspensionMaxDrop;
		PSuspensionData.mSpringStrength = FMath::Square( Wheel->SuspensionNaturalFrequency ) * PSuspensionData.mSprungMass;
		PSuspensionData.mSpringDamperRate = Wheel->SuspensionDampingRatio * 2.0f * FMath::Sqrt( PSuspensionData.mSpringStrength * PSuspensionData.mSprungMass );

		// init offsets
		const PxVec3 PWheelOffset = WheelOffsets[WheelIdx];

		PxVec3 PSuspTravelDirection = PxVec3( 0.0f, 0.0f, -1.0f );
		PxVec3 PWheelCentreCMOffset = PWheelOffset - PCOMOffset;
		PxVec3 PSuspForceAppCMOffset = PxVec3( PWheelOffset.x, PWheelOffset.y, Wheel->SuspensionForceOffset );
		PxVec3 PTireForceAppCMOffset = PSuspForceAppCMOffset;

		// finalize sim data
		PWheelsSimData->setWheelData( WheelIdx, PWheelData );
		PWheelsSimData->setTireData( WheelIdx, PTireData );
		PWheelsSimData->setSuspensionData( WheelIdx, PSuspensionData );
		PWheelsSimData->setSuspTravelDirection( WheelIdx, PSuspTravelDirection );
		PWheelsSimData->setWheelCentreOffset( WheelIdx, PWheelCentreCMOffset );
		PWheelsSimData->setSuspForceAppPointOffset( WheelIdx, PSuspForceAppCMOffset );
		PWheelsSimData->setTireForceAppPointOffset( WheelIdx, PTireForceAppCMOffset );
	}

	const int32 NumShapes = PVehicleActor->getNbShapes();
	const int32 NumChassisShapes = NumShapes - WheelSetups.Num();
	check( NumChassisShapes >= 1 );

	TArray<PxShape*> Shapes;
	Shapes.AddZeroed(NumShapes);

	PVehicleActor->getShapes( Shapes.GetTypedData(), NumShapes );

	for ( int32 WheelIdx = 0; WheelIdx < WheelSetups.Num(); ++WheelIdx )
	{
		const int32 WheelShapeIndex = NumChassisShapes + WheelIdx;

		PWheelsSimData->setWheelShapeMapping( WheelIdx, WheelShapeIndex );
		PWheelsSimData->setSceneQueryFilterData( WheelIdx, Shapes[WheelShapeIndex]->getQueryFilterData());
	}
}
 ////////////////////////////////////////////////////////////////////////////
//Default tire force shader function.
//Taken from Michigan tire model.
//Computes tire long and lat forces plus the aligning moment arising from 
//the lat force and the torque to apply back to the wheel arising from the 
//long force (application of Newton's 3rd law).
////////////////////////////////////////////////////////////////////////////


#define ONE_TWENTYSEVENTH 0.037037f
#define ONE_THIRD 0.33333f
PX_FORCE_INLINE PxF32 smoothingFunction1(const PxF32 K)
{
	//Equation 20 in CarSimEd manual Appendix F.
	//Looks a bit like a curve of sqrt(x) for 0<x<1 but reaching 1.0 on y-axis at K=3. 
	PX_ASSERT(K>=0.0f);
	return PxMin(1.0f, K - ONE_THIRD*K*K + ONE_TWENTYSEVENTH*K*K*K);
}
PX_FORCE_INLINE PxF32 smoothingFunction2(const PxF32 K)
{
	//Equation 21 in CarSimEd manual Appendix F.
	//Rises to a peak at K=0.75 and falls back to zero by K=3
	PX_ASSERT(K>=0.0f);
	return (K - K*K + ONE_THIRD*K*K*K - ONE_TWENTYSEVENTH*K*K*K*K);
}

void PxVehicleComputeTireForceDefault
(const void* tireShaderData, 
 const PxF32 tireFriction,
 const PxF32 longSlip, const PxF32 latSlip, const PxF32 camber,
 const PxF32 wheelOmega, const PxF32 wheelRadius, const PxF32 recipWheelRadius,
 const PxF32 restTireLoad, const PxF32 normalisedTireLoad, const PxF32 tireLoad,
 const PxF32 gravity, const PxF32 recipGravity,
 PxF32& wheelTorque, PxF32& tireLongForceMag, PxF32& tireLatForceMag, PxF32& tireAlignMoment)
{
	PX_UNUSED(wheelOmega);
	PX_UNUSED(recipWheelRadius);

	const PxVehicleTireData& tireData=*((PxVehicleTireData*)tireShaderData);

	PX_ASSERT(tireFriction>0);
	PX_ASSERT(tireLoad>0);

	wheelTorque=0.0f;
	tireLongForceMag=0.0f;
	tireLatForceMag=0.0f;
	tireAlignMoment=0.0f;

	//If long slip/lat slip/camber are all zero than there will be zero tire force.
	if((0==latSlip)&&(0==longSlip)&&(0==camber))
	{
		return;
	}

	//Compute the lateral stiffness
	const PxF32 latStiff=restTireLoad*tireData.mLatStiffY*smoothingFunction1(normalisedTireLoad*3.0f/tireData.mLatStiffX);

	//Get the longitudinal stiffness
	const PxF32 longStiff=tireData.mLongitudinalStiffnessPerUnitGravity*gravity;
	const PxF32 recipLongStiff=tireData.getRecipLongitudinalStiffnessPerUnitGravity()*recipGravity;

	//Get the camber stiffness.
	const PxF32 camberStiff=tireData.mCamberStiffnessPerUnitGravity*gravity;

	//Carry on and compute the forces.
	const PxF32 TEff = PxTan(latSlip - camber*camberStiff/latStiff);
	const PxF32 K = PxSqrt(latStiff*TEff*latStiff*TEff + longStiff*longSlip*longStiff*longSlip) /(tireFriction*tireLoad);
	//const PxF32 KAbs=PxAbs(K);
	PxF32 FBar = smoothingFunction1(K);//K - ONE_THIRD*PxAbs(K)*K + ONE_TWENTYSEVENTH*K*K*K;
	PxF32 MBar = smoothingFunction2(K); //K - KAbs*K + ONE_THIRD*K*K*K - ONE_TWENTYSEVENTH*KAbs*K*K*K;
	//Mbar = PxMin(Mbar, 1.0f);
	PxF32 nu=1;
	if(K <= 2.0f*PxPi)
	{
		const PxF32 latOverlLong=latStiff*recipLongStiff;
		nu = 0.5f*(1.0f + latOverlLong - (1.0f - latOverlLong)*PxCos(K*0.5f));
	}
	const PxF32 FZero = tireFriction*tireLoad / (PxSqrt(longSlip*longSlip + nu*TEff*nu*TEff));
	const PxF32 fz = longSlip*FBar*FZero;
	const PxF32 fx = -nu*TEff*FBar*FZero;
	//TODO: pneumatic trail.
	const PxF32 pneumaticTrail=1.0f;
	const PxF32	fMy= nu * pneumaticTrail * TEff * MBar * FZero;

	//We can add the torque to the wheel.
	wheelTorque=-fz*wheelRadius;
	tireLongForceMag=fz;
	tireLatForceMag=fx;
	tireAlignMoment=fMy;
}

void UWheeledVehicleMovementComponent::GenerateTireForces( UVehicleWheel* Wheel, const FTireShaderInput& Input, FTireShaderOutput& Output )
{
	const void* realShaderData = &PVehicle->mWheelsSimData.getTireData(Wheel->WheelIndex);

	float Dummy;

	PxVehicleComputeTireForceDefault(
		realShaderData, Input.TireFriction,
		Input.LongSlip, Input.LatSlip,
		0.0f, Input.WheelOmega, Input.WheelRadius, Input.RecipWheelRadius,
		Input.RestTireLoad, Input.NormalizedTireLoad, Input.TireLoad,
		Input.Gravity, Input.RecipGravity,
		Output.WheelTorque, Output.LongForce, Output.LatForce, Dummy
		);
	
	ensureMsgf(Output.WheelTorque == Output.WheelTorque, TEXT("Output.WheelTorque is bad: %f"), Output.WheelTorque);
	ensureMsgf(Output.LongForce == Output.LongForce, TEXT("Output.LongForce is bad: %f"), Output.LongForce);
	ensureMsgf(Output.LatForce == Output.LatForce, TEXT("Output.LatForce is bad: %f"), Output.LatForce);

	//UE_LOG( LogVehicles, Warning, TEXT("Friction = %f	LongSlip = %f	LatSlip = %f"), Input.TireFriction, Input.LongSlip, Input.LatSlip );	
	//UE_LOG( LogVehicles, Warning, TEXT("WheelTorque= %f	LongForce = %f	LatForce = %f"), Output.WheelTorque, Output.LongForce, Output.LatForce );
	//UE_LOG( LogVehicles, Warning, TEXT("RestLoad= %f	NormLoad = %f	TireLoad = %f"),Input.RestTireLoad, Input.NormalizedTireLoad, Input.TireLoad );
	
	//Output.LatForce *= FMath::Lerp( Wheel->WheelSetup->LatStiffFactor, Wheel->WheelSetup->HandbrakeLatStiffFactor, OutputHandbrake );
	//Output.LatForce *= Wheel->LatStiffFactor;
}

void UWheeledVehicleMovementComponent::PostSetupVehicle()
{
}

FVector UWheeledVehicleMovementComponent::GetWheelRestingPosition( const FWheelSetup& WheelSetup )
{
	FVector Offset = WheelSetup.WheelClass.GetDefaultObject()->Offset + WheelSetup.AdditionalOffset;

	if ( WheelSetup.BoneName != NAME_None )
	{
		USkinnedMeshComponent* Mesh = GetMesh();

		if ( Mesh && Mesh->SkeletalMesh )
		{
			const FVector BonePosition = Mesh->SkeletalMesh->GetComposedRefPoseMatrix( WheelSetup.BoneName ).GetOrigin() * Mesh->RelativeScale3D;
			Offset += BonePosition;
		}
	}

	return Offset;
}

FVector UWheeledVehicleMovementComponent::GetCOMOffset()
{
	return COMOffset;
}

USkinnedMeshComponent* UWheeledVehicleMovementComponent::GetMesh()
{
	return Cast<USkinnedMeshComponent>(UpdatedComponent);
}

void LogVehicleSettings( PxVehicleWheels* Vehicle )
{
	const float VehicleMass = Vehicle->getRigidDynamicActor()->getMass();
	const FVector VehicleMOI = P2UVector( Vehicle->getRigidDynamicActor()->getMassSpaceInertiaTensor() );

	UE_LOG( LogPhysics, Warning, TEXT("Vehicle Mass: %f"), VehicleMass );
	UE_LOG( LogPhysics, Warning, TEXT("Vehicle MOI: %s"), *VehicleMOI.ToString() );

	const PxVehicleWheelsSimData& SimData = Vehicle->mWheelsSimData;
	for ( int32 WheelIdx = 0; WheelIdx < 4; ++WheelIdx )
	{
		const  PxVec3& suspTravelDir = SimData.getSuspTravelDirection(WheelIdx);
		const PxVec3& suspAppPointOffset = SimData.getSuspForceAppPointOffset(WheelIdx);
		const PxVec3& tireForceAppPointOffset = SimData.getTireForceAppPointOffset(WheelIdx);
		const PxVec3& wheelCenterOffset = SimData.getWheelCentreOffset(WheelIdx);			
		const PxVehicleSuspensionData& SuspensionData = SimData.getSuspensionData( WheelIdx );
		const PxVehicleWheelData& WheelData = SimData.getWheelData( WheelIdx );
		const PxVehicleTireData& TireData = SimData.getTireData( WheelIdx );

		UE_LOG( LogPhysics, Warning, TEXT("Wheel %d suspension: travelDir ={%f, %f, %f} "), WheelIdx, suspTravelDir.x, suspTravelDir.y, suspTravelDir.z );
		UE_LOG( LogPhysics, Warning, TEXT("Wheel %d suspension: suspAppPointOffset ={%f, %f, %f} "), WheelIdx, suspAppPointOffset.x, suspAppPointOffset.y, suspAppPointOffset.z );
		UE_LOG( LogPhysics, Warning, TEXT("Wheel %d suspension: tireForceAppPointOffset ={%f, %f, %f} "), WheelIdx, tireForceAppPointOffset.x, tireForceAppPointOffset.y, tireForceAppPointOffset.z );
		UE_LOG( LogPhysics, Warning, TEXT("Wheel %d suspension: wheelCenterOffset ={%f, %f, %f} "), WheelIdx, wheelCenterOffset.x, wheelCenterOffset.y, wheelCenterOffset.z );
		UE_LOG( LogPhysics, Warning, TEXT("Wheel %d suspension: MaxCompress=%f, MaxDroop=%f, Damper=%f, Strength=%f, SprungMass=%f"),
			WheelIdx, SuspensionData.mMaxCompression, SuspensionData.mMaxDroop, SuspensionData.mSpringDamperRate, SuspensionData.mSpringStrength, SuspensionData.mSprungMass );

		UE_LOG( LogPhysics, Warning, TEXT("Wheel %d wheel: Damping=%f, Mass=%f, MOI=%f, Radius=%f"),
			WheelIdx, WheelData.mDampingRate, WheelData.mMass, WheelData.mMOI, WheelData.mRadius );

		UE_LOG( LogPhysics, Warning, TEXT("Wheel %d tire: LatStiffX=%f, LatStiffY=%f, LongStiff=%f"),
			WheelIdx, TireData.mLatStiffX, TireData.mLatStiffY, TireData.mLongitudinalStiffnessPerUnitGravity );
	}
}

void UWheeledVehicleMovementComponent::CreatePhysicsState()
{
	Super::CreatePhysicsState();

	VehicleSetupTag = FPhysXVehicleManager::VehicleSetupTag;

	// only create physx vehicle in game
	if ( GetWorld()->IsGameWorld() )
	{
		FPhysScene* PhysScene = World->GetPhysicsScene();

		if ( PhysScene && PhysScene->GetVehicleManager() )
		{
			CreateVehicle();

			if ( PVehicle )
			{
				PhysScene->GetVehicleManager()->AddVehicle( this );

				CreateWheels();

				//LogVehicleSettings( PVehicle );

				PVehicle->getRigidDynamicActor()->wakeUp();
			}
		}
	}
}

void UWheeledVehicleMovementComponent::DestroyPhysicsState()
{
	Super::DestroyPhysicsState();

	if ( PVehicle )
	{
		DestroyWheels();

		World->GetPhysicsScene()->GetVehicleManager()->RemoveVehicle( this );
		PVehicle = NULL;

		if ( UpdatedComponent )
		{
			UpdatedComponent->RecreatePhysicsState();
		}
	}
}

void UWheeledVehicleMovementComponent::CreateWheels()
{
	// Wheels num is getting copied when blueprint recompiles, so we have to manually reset here
	Wheels.Reset();

	PVehicle->mWheelsDynData.setTireForceShaderFunction( PTireShader );

	// Instantiate the wheels
	for ( int32 WheelIdx = 0; WheelIdx < WheelSetups.Num(); ++WheelIdx )
	{
		UVehicleWheel* Wheel = NewObject<UVehicleWheel>( this, WheelSetups[WheelIdx].WheelClass );
		check(Wheel);

		Wheels.Add( Wheel );
	}

	// Initialize the wheels
	for ( int32 WheelIdx = 0; WheelIdx < Wheels.Num(); ++WheelIdx )
	{
		PVehicle->mWheelsDynData.setTireForceShaderData( WheelIdx, Wheels[WheelIdx] );

		Wheels[WheelIdx]->Init( this, WheelIdx );
	}
}

void UWheeledVehicleMovementComponent::DestroyWheels()
{
	for ( int32 i = 0; i < Wheels.Num(); ++i )
	{
		Wheels[i]->Shutdown();
	}

	Wheels.Reset();
}

void UWheeledVehicleMovementComponent::TickVehicle( float DeltaTime )
{
	if (VehicleSetupTag != FPhysXVehicleManager::VehicleSetupTag )
	{
		RecreatePhysicsState();
	}

	// movement updates and replication
	if (PVehicle && UpdatedComponent)
	{
		APawn* MyOwner = Cast<APawn>(UpdatedComponent->GetOwner());
		if (MyOwner)
		{
			UpdateState(DeltaTime);
			UpdateSimulation(DeltaTime);
		}
	}

	// update wheels
	for (int32 i = 0; i < Wheels.Num(); i++)
	{
		Wheels[i]->Tick(DeltaTime);
	}
}

void UWheeledVehicleMovementComponent::UpdateSimulation( float DeltaTime )
{
}

#endif // WITH_PHYSX

void UWheeledVehicleMovementComponent::UpdateState( float DeltaTime )
{
	// update input values
	APawn* MyOwner = UpdatedComponent ? Cast<APawn>(UpdatedComponent->GetOwner()) : NULL;
	if (MyOwner && MyOwner->IsLocallyControlled())
	{
		SteeringInput = SteeringInputRate.InterpInputValue( DeltaTime, SteeringInput, CalcSteeringInput() );
		ThrottleInput = ThrottleInputRate.InterpInputValue( DeltaTime, ThrottleInput, CalcThrottleInput() );
		BrakeInput = BrakeInputRate.InterpInputValue( DeltaTime, BrakeInput, CalcBrakeInput() );
		HandbrakeInput = HandbrakeInputRate.InterpInputValue( DeltaTime, HandbrakeInput, CalcHandbrakeInput() );

		// and send to server
		ServerUpdateState(SteeringInput, ThrottleInput, BrakeInput, HandbrakeInput, GetCurrentGear());
	}
	else
	{
		// use replicated values for remote pawns
		SteeringInput = ReplicatedState.SteeringInput;
		ThrottleInput = ReplicatedState.ThrottleInput;
		BrakeInput = ReplicatedState.BrakeInput;
		HandbrakeInput = ReplicatedState.HandbrakeInput;
		SetTargetGear(ReplicatedState.CurrentGear, true);
	}
}

bool UWheeledVehicleMovementComponent::ServerUpdateState_Validate(float InSteeringInput, float InThrottleInput, float InBrakeInput, float InHandbrakeInput, int32 InCurrentGear)
{
	return true;
}

void UWheeledVehicleMovementComponent::ServerUpdateState_Implementation(float InSteeringInput, float InThrottleInput, float InBrakeInput, float InHandbrakeInput, int32 InCurrentGear)
{
	SteeringInput = InSteeringInput;
	ThrottleInput = InThrottleInput;
	BrakeInput = InBrakeInput;
	HandbrakeInput = InHandbrakeInput;

	if (!GetUseAutoGears())
	{
		SetTargetGear(InCurrentGear, true);
	}

	// update state of inputs
	ReplicatedState.SteeringInput = InSteeringInput;
	ReplicatedState.ThrottleInput = InThrottleInput;
	ReplicatedState.BrakeInput = InBrakeInput;
	ReplicatedState.HandbrakeInput = InHandbrakeInput;
	ReplicatedState.CurrentGear = InCurrentGear;
}

float UWheeledVehicleMovementComponent::CalcSteeringInput()
{
	return RawSteeringInput;
}

float UWheeledVehicleMovementComponent::CalcBrakeInput()
{	
	const float ForwardSpeed = GetForwardSpeed();

	float NewBrakeInput = 0.0f;

	// if player wants to move forwards...
	if ( RawThrottleInput > 0.1f )  // MSS expanded hardcoded dead zone from 0.01f to filter weird throttle noise.
	{
		// if vehicle is moving backwards, then press brake
		if ( ForwardSpeed < -StopThreshold )
		{
			NewBrakeInput = 1.0f;			
			SetTargetGear(1, true);
		}

	}

	// if player wants to move backwards...
	else if ( RawThrottleInput < -0.1f )
	{
		// if vehicle is moving forwards, then press brake
		if ( ForwardSpeed > StopThreshold )
		{
			NewBrakeInput = 1.0f;			// Seems a bit severe to have 0 or 1 braking. Better control can be had by allowing continuous brake input values
			// set to reverse
			SetTargetGear(-1, true);	
		}

		
	}

	// if player isn't pressing forward or backwards...
	else
	{
		// If almost stationary, stick brakes on
		if ( FMath::Abs(ForwardSpeed) <  StopThreshold )
		{
			NewBrakeInput = 0.1f + 0.1f * FMath::Abs(ForwardSpeed);
		}
		else
		{
			NewBrakeInput = IdleBrakeInput;
		}
	}

	return FMath::Clamp<float>(NewBrakeInput, 0.0, 1.0);
}

float UWheeledVehicleMovementComponent::CalcHandbrakeInput()
{
	return (bRawHandbrakeInput == true) ? 1.0f : 0.0f;
}

float UWheeledVehicleMovementComponent::CalcThrottleInput()
{
	return FMath::Abs(RawThrottleInput);
}

void UWheeledVehicleMovementComponent::ClearInput()
{
	SteeringInput = 0.0f;
	ThrottleInput = 0.0f;
	BrakeInput = 0.0f;
	HandbrakeInput = 0.0f;
}

void UWheeledVehicleMovementComponent::SetThrottleInput( float Throttle )
{	
	RawThrottleInput = FMath::Clamp( Throttle, -1.0f, 1.0f );
}

void UWheeledVehicleMovementComponent::SetSteeringInput( float Steering )
{
	RawSteeringInput = FMath::Clamp( Steering, -1.0f, 1.0f );
}

void UWheeledVehicleMovementComponent::SetHandbrakeInput( bool bNewHandbrake )
{
	bRawHandbrakeInput = bNewHandbrake;
}

void UWheeledVehicleMovementComponent::SetGearUp(bool bNewGearUp)
{
	bRawGearUpInput = bNewGearUp;
}

void UWheeledVehicleMovementComponent::SetGearDown(bool bNewGearDown)
{
	bRawGearDownInput = bNewGearDown;
}

void UWheeledVehicleMovementComponent::SetTargetGear(int32 GearNum, bool bImmediate)
{
#if WITH_PHYSX
	//UE_LOG( LogVehicles, Warning, TEXT(" UWheeledVehicleMovementComponent::SetTargetGear::GearNum = %d, bImmediate = %d"), GearNum, bImmediate);
	const uint32 TargetGearNum = GearToPhysXGear(GearNum);
	if (PVehicleDrive && PVehicleDrive->mDriveDynData.getTargetGear() != TargetGearNum)
	{
		if (bImmediate)
		{
			PVehicleDrive->mDriveDynData.forceGearChange(TargetGearNum);			
		}
		else
		{
			PVehicleDrive->mDriveDynData.startGearChange(TargetGearNum);
		}
	}
#endif
}

void UWheeledVehicleMovementComponent::SetUseAutoGears(bool bUseAuto)
{
#if WITH_PHYSX
	if (PVehicleDrive)
	{
		PVehicleDrive->mDriveDynData.setUseAutoGears(bUseAuto);
	}
#endif
}

float UWheeledVehicleMovementComponent::GetForwardSpeed() const
{
#if WITH_PHYSX
	if ( PVehicle )
	{
		return PVehicle->computeForwardSpeed();
	}
#endif

	return 0.0f;
}

float UWheeledVehicleMovementComponent::GetEngineRotationSpeed() const
{
#if WITH_PHYSX
	if (PVehicleDrive)
	{		
		return 9.5493 *  PVehicleDrive->mDriveDynData.getEngineRotationSpeed(); // 9.5493 = 60sec/min * (Motor Omega)/(2 * Pi); Motor Omega is in radians/sec, not RPM.
	}
	else if (PVehicle && WheelSetups.Num())
	{
		float TotalWheelSpeed = 0.0f;

		for (int32 i = 0; i < WheelSetups.Num(); i++)
		{
			const PxReal WheelSpeed = PVehicle->mWheelsDynData.getWheelRotationSpeed(i);
			TotalWheelSpeed += WheelSpeed;
		}

		const float CurrentRPM = TotalWheelSpeed / WheelSetups.Num();
		return CurrentRPM;
	}
#endif

	return 0.0f;
}

#if WITH_PHYSX

int32 UWheeledVehicleMovementComponent::GearToPhysXGear(const int32 Gear) const
{
	if (Gear < 0)
	{
		return PxVehicleGearsData::eREVERSE;
	}
	else if (Gear == 0)
	{
		return PxVehicleGearsData::eNEUTRAL;
	}

	return FMath::Min(PxVehicleGearsData::eNEUTRAL + Gear, PxVehicleGearsData::eGEARSRATIO_COUNT - 1);
}

int32 UWheeledVehicleMovementComponent::PhysXGearToGear(const int32 PhysXGear) const
{
	if (PhysXGear == PxVehicleGearsData::eREVERSE)
	{
		return -1;
	}
	else if (PhysXGear == PxVehicleGearsData::eNEUTRAL)
	{
		return 0;
	}

	return (PhysXGear - PxVehicleGearsData::eNEUTRAL);

}

#endif

int32 UWheeledVehicleMovementComponent::GetCurrentGear() const
{
#if WITH_PHYSX
	if (PVehicleDrive)
	{
		const int32 PhysXGearNum = PVehicleDrive->mDriveDynData.getCurrentGear();
		return PhysXGearToGear(PhysXGearNum);
	}
#endif

	return 0;
}

int32 UWheeledVehicleMovementComponent::GetTargetGear() const
{
#if WITH_PHYSX
	if (PVehicleDrive)
	{
		const int32 PhysXGearNum = PVehicleDrive->mDriveDynData.getTargetGear();
		return PhysXGearToGear(PhysXGearNum);
	}
#endif

	return 0;
}

bool UWheeledVehicleMovementComponent::GetUseAutoGears() const
{
#if WITH_PHYSX
	if (PVehicleDrive)
	{
		return PVehicleDrive->mDriveDynData.getUseAutoGears();
	}
#endif

	return false;
}

#if WITH_PHYSX

void DrawTelemetryGraph( uint32 Channel, const PxVehicleGraph& PGraph, UCanvas* Canvas, float GraphX, float GraphY, float GraphWidth, float GraphHeight )
{
	PxF32 PGraphXY[2*PxVehicleGraph::eMAX_NB_SAMPLES];
	PxVec3 PGraphColor[PxVehicleGraph::eMAX_NB_SAMPLES];
	char PGraphTitle[PxVehicleGraph::eMAX_NB_TITLE_CHARS];

	PGraph.computeGraphChannel( Channel, PGraphXY, PGraphColor, PGraphTitle );

	FString Label = ANSI_TO_TCHAR(PGraphTitle);
	Canvas->SetDrawColor( FColor( 128, 255, 0 ) );
	UFont* Font = GEngine->GetSmallFont();
	Canvas->DrawText( Font, Label, GraphX, GraphY );

	float XL, YL;
	Canvas->TextSize( Font, Label, XL, YL );

	float LineGraphHeight = GraphHeight - YL - 4.0f;
	float LineGraphY = GraphY + YL + 4.0f;

	FCanvasTileItem TileItem( FVector2D(GraphX, LineGraphY), GWhiteTexture, FVector2D( GraphWidth, GraphWidth ), FLinearColor( 0.0f, 0.125f, 0.0f, 0.5f ) );
	Canvas->DrawItem( TileItem );
	
	Canvas->SetDrawColor( FColor( 0, 32, 0, 128 ) );	
	for ( uint32 i = 2; i < 2 * PxVehicleGraph::eMAX_NB_SAMPLES; i += 2 )
	{
		float x1 = PGraphXY[i-2];
		float y1 = PGraphXY[i-1];
		float x2 = PGraphXY[i];
		float y2 = PGraphXY[i+1];

		x1 = FMath::Clamp( x1 + 0.50f, 0.0f, 1.0f );
		x2 = FMath::Clamp( x2 + 0.50f, 0.0f, 1.0f );
		y1 = 1.0f - FMath::Clamp( y1 + 0.50f, 0.0f, 1.0f );
		y2 = 1.0f - FMath::Clamp( y2 + 0.50f, 0.0f, 1.0f );

		FCanvasLineItem LineItem( FVector2D( GraphX + x1 * GraphWidth, LineGraphY + y1 * LineGraphHeight ), FVector2D( GraphX + x2 * GraphWidth, LineGraphY + y2 * LineGraphHeight ) );
		LineItem.SetColor( FLinearColor( 1.0f, 0.5f, 0.0f, 1.0f ) );
		LineItem.Draw( Canvas->Canvas );
	}
}

void UWheeledVehicleMovementComponent::DrawDebug( UCanvas* Canvas, float& YL, float& YPos )
{
	if ( PVehicle == NULL )
	{
		return;
	}
	
	FPhysXVehicleManager* MyVehicleManager = World->GetPhysicsScene()->GetVehicleManager();

	MyVehicleManager->SetRecordTelemetry( this, true );

	UFont* RenderFont = GEngine->GetSmallFont();
	// draw drive data
	{
		Canvas->SetDrawColor( FColor::White );

		YPos += Canvas->DrawText( RenderFont, FString::Printf( TEXT("Speed: %d"), (int32)GetForwardSpeed() ), 4, YPos );
		YPos += Canvas->DrawText( RenderFont, FString::Printf( TEXT("Steering: %f"), SteeringInput ), 4, YPos  );
		YPos += Canvas->DrawText( RenderFont, FString::Printf( TEXT("Throttle: %f"), ThrottleInput ), 4, YPos );
		YPos += Canvas->DrawText( RenderFont, FString::Printf( TEXT("Brake: %f"), BrakeInput ), 4, YPos  );

	}

	PxWheelQueryResult* WheelsStates = MyVehicleManager->GetWheelsStates(this);
	check(WheelsStates);

	// draw wheel data
	for ( uint32 w = 0; w < PVehicle->mWheelsSimData.getNbWheels(); ++w )
	{

		UVehicleWheel* Wheel = Wheels[w];
		const PxMaterial* ContactSurface = WheelsStates[w].tireSurfaceMaterial;
		const PxReal TireFriction = WheelsStates[w].tireFriction;
		const PxReal LatSlip = WheelsStates[w].lateralSlip;
		const PxReal LongSlip = WheelsStates[w].longitudinalSlip;
		const PxReal WheelSpeed = PVehicle->mWheelsDynData.getWheelRotationSpeed(w) * PVehicle->mWheelsSimData.getWheelData(w).mRadius;

		UPhysicalMaterial* ContactSurfaceMaterial = ContactSurface ? FPhysxUserData::Get<UPhysicalMaterial>(ContactSurface->userData) : NULL;
		const FString ContactSurfaceString = ContactSurfaceMaterial ? ContactSurfaceMaterial->GetName() : FString(TEXT("NONE"));

		Canvas->SetDrawColor( FColor::White );

		Canvas->DrawText( RenderFont, FString::Printf( TEXT("[%d]"), w ), 4, YPos );
		
		Canvas->DrawText( RenderFont, FString::Printf( TEXT("LatSlip: %.3f"), LatSlip ), YL * 4 , YPos );
		Canvas->DrawText( RenderFont, FString::Printf( TEXT("LongSlip: %.3f"), LongSlip ), YL * 12, YPos );
		Canvas->DrawText( RenderFont, FString::Printf( TEXT("Speed: %d"), (int32)WheelSpeed ), YL * 20, YPos );
		Canvas->DrawText( RenderFont, FString::Printf( TEXT("Contact Surface: %s"), *ContactSurfaceString ), YL * 60, YPos );
		if( (int32)w < Wheels.Num() )
		{
			UVehicleWheel* Wheel = Wheels[w];
			Canvas->DrawText( RenderFont, FString::Printf( TEXT("Load: %f"), (int32)Wheel->DebugNormalizedTireLoad ), YL * 28, YPos );
			Canvas->DrawText( RenderFont, FString::Printf( TEXT("Torque: %d"), (int32)Wheel->DebugWheelTorque ), YL * 36, YPos );
			Canvas->DrawText( RenderFont, FString::Printf( TEXT("Long Force: %d"), (int32)Wheel->DebugLongForce ),YL * 44, YPos );
			Canvas->DrawText( RenderFont, FString::Printf( TEXT("Lat Force: %d"), (int32)Wheel->DebugLatForce ), YL * 52, YPos );
		}
		else
		{
			Canvas->DrawText( RenderFont, TEXT("Wheels array insufficiently sized!"), YL * 72, YPos );
		}

		YPos += YL;
	}

	// draw wheel graphs
	PxVehicleTelemetryData* TelemetryData = MyVehicleManager->GetTelemetryData();

	if ( TelemetryData )
	{
		const float GraphWidth(100.0f), GraphHeight(100.0f);
	
			int GraphChannels[] = {
			PxVehicleWheelGraphChannel::eNORMALIZED_TIRELOAD,
			PxVehicleWheelGraphChannel::eWHEEL_OMEGA,
			PxVehicleWheelGraphChannel::eTIRE_LONG_SLIP,
			PxVehicleWheelGraphChannel::eNORM_TIRE_LONG_FORCE,
			PxVehicleWheelGraphChannel::eTIRE_LAT_SLIP,
			PxVehicleWheelGraphChannel::eNORM_TIRE_LAT_FORCE
		};

		for ( uint32 w = 0; w < PVehicle->mWheelsSimData.getNbWheels(); ++w )
		{
			float CurX = 4;
			for ( uint32 i = 0; i < ARRAY_COUNT(GraphChannels); ++i )
			{
				DrawTelemetryGraph( GraphChannels[i], TelemetryData->getWheelGraph(w), Canvas, CurX, YPos, GraphWidth, GraphHeight );
				CurX += GraphWidth + 10.0f;
			}

			YPos += GraphHeight;
			YPos += YL;
		}
	}
}

void UWheeledVehicleMovementComponent::DrawDebugLines()
{
	if ( PVehicle == NULL )
	{
		return;
	}

	FPhysXVehicleManager* MyVehicleManager = World->GetPhysicsScene()->GetVehicleManager();

	MyVehicleManager->SetRecordTelemetry( this, true );

	PxRigidDynamic* PActor = PVehicle->getRigidDynamicActor();

	// gather wheel shapes
	PxShape* PShapeBuffer[32];
	PActor->getShapes( PShapeBuffer, 32 );
	const uint32 PNumWheels = PVehicle->mWheelsSimData.getNbWheels();

	UWorld* World = GetWorld();

	// draw chassis orientation
	const PxTransform GlobalT = PActor->getGlobalPose();
	const PxTransform T = GlobalT.transform( PActor->getCMassLocalPose() );
	const PxVec3 ChassisExtent = PActor->getWorldBounds().getExtents();
	const float ChassisSize = ChassisExtent.magnitude();
	DrawDebugLine( World, P2UVector(T.p), P2UVector( T.p + T.rotate( PxVec3( ChassisSize, 0, 0 ) ) ), FColor(255,0,0) );
	DrawDebugLine( World, P2UVector(T.p), P2UVector( T.p + T.rotate( PxVec3( 0, ChassisSize, 0 ) ) ), FColor(0,255,0) );
	DrawDebugLine( World, P2UVector(T.p), P2UVector( T.p + T.rotate( PxVec3( 0, 0, ChassisSize ) ) ), FColor(0,0,255) );

	PxVehicleTelemetryData* TelemetryData = MyVehicleManager->GetTelemetryData();
	
	PxWheelQueryResult* WheelsStates = MyVehicleManager->GetWheelsStates(this);
	check(WheelsStates);

	for ( uint32 w = 0; w < PNumWheels; ++w )
	{
		// render suspension raycast
	
		const FVector SuspensionStart = P2UVector( WheelsStates[w].suspLineStart );
		const FVector SuspensionEnd = P2UVector( WheelsStates[w].suspLineStart + WheelsStates[w].suspLineDir * WheelsStates[w].suspLineLength );
		const FColor SuspensionColor = WheelsStates[w].tireSurfaceMaterial == NULL ? FColor(255,64,64) : FColor(64,255,64);
		DrawDebugLine(World, SuspensionStart, SuspensionEnd, SuspensionColor );

		// render wheel radii
		const int32 ShapeIndex = PVehicle->mWheelsSimData.getWheelShapeMapping( w );
		const PxF32 WheelRadius = PVehicle->mWheelsSimData.getWheelData(w).mRadius;
		const PxF32 WheelWidth = PVehicle->mWheelsSimData.getWheelData(w).mWidth;
		const FTransform WheelTransform = P2UTransform( PActor->getGlobalPose().transform( PShapeBuffer[ShapeIndex]->getLocalPose() ) );
		const FVector WheelLocation = WheelTransform.GetLocation();
		const FVector WheelLatDir = WheelTransform.TransformVector( FVector( 0.0f, 1.0f, 0.0f ) );
		const FVector WheelLatOffset = WheelLatDir * WheelWidth * 0.50f;
		//const FVector WheelRotDir = FQuat( WheelLatDir, PVehicle->mWheelsDynData.getWheelRotationAngle(w) ) * FVector( 1.0f, 0.0f, 0.0f );
		const FVector WheelRotDir = WheelTransform.TransformVector( FVector( 1.0f, 0.0f, 0.0f ) );
		const FVector WheelRotOffset = WheelRotDir * WheelRadius;

		const FVector CylinderStart = WheelLocation + WheelLatOffset;
		const FVector CylinderEnd = WheelLocation - WheelLatOffset;

		DrawDebugCylinder( World, CylinderStart, CylinderEnd, WheelRadius, 16, SuspensionColor );
		DrawDebugLine( World, WheelLocation, WheelLocation + WheelRotOffset, SuspensionColor );

		const FVector ContactPoint = P2UVector( WheelsStates[w].tireContactPoint );
		DrawDebugBox( World, ContactPoint, FVector(4.0f), FQuat::Identity, SuspensionColor );

		if ( TelemetryData )
		{
			// Draw all tire force app points.
			const PxVec3& PAppPoint = TelemetryData->getTireforceAppPoints()[w];
			const FVector AppPoint = P2UVector( PAppPoint );
			DrawDebugBox( World, AppPoint, FVector(5.0f), FQuat::Identity, FColor( 255, 0, 255 ) );

			// Draw all susp force app points.
			const PxVec3& PAppPoint2 = TelemetryData->getSuspforceAppPoints()[w];
			const FVector AppPoint2 = P2UVector( PAppPoint2 );
			DrawDebugBox( World, AppPoint2, FVector(5.0f), FQuat::Identity, FColor( 0, 255, 255 ) );
		}
	}
}

#if WITH_EDITOR

void UWheeledVehicleMovementComponent::PostEditChangeProperty( FPropertyChangedEvent& PropertyChangedEvent )
{
	Super::PostEditChangeProperty( PropertyChangedEvent );

	// Trigger a runtime rebuild of the PhysX vehicle
	FPhysXVehicleManager::VehicleSetupTag++;
}

#endif // WITH_EDITOR

#endif // WITH_PHYSX

void UWheeledVehicleMovementComponent::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( UWheeledVehicleMovementComponent, ReplicatedState );
}
