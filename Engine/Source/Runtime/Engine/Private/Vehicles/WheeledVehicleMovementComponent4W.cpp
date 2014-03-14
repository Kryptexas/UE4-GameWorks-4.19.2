// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

#if WITH_PHYSX
#include "../PhysicsEngine/PhysXSupport.h"
#endif // WITH_PHYSX

UWheeledVehicleMovementComponent4W::UWheeledVehicleMovementComponent4W(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
#if WITH_PHYSX
	// grab default values from physx
	PxVehicleDifferential4WData DefDifferentialSetup;
	TEnumAsByte<EVehicleDifferential4W::Type> DiffType((uint8)DefDifferentialSetup.mType);
	DifferentialSetup.DifferentialType = DiffType;
	DifferentialSetup.FrontRearSplit = DefDifferentialSetup.mFrontRearSplit;
	DifferentialSetup.FrontLeftRightSplit = DefDifferentialSetup.mFrontLeftRightSplit;
	DifferentialSetup.RearLeftRightSplit = DefDifferentialSetup.mRearLeftRightSplit;
	DifferentialSetup.CentreBias = DefDifferentialSetup.mCentreBias;
	DifferentialSetup.FrontBias = DefDifferentialSetup.mFrontBias;
	DifferentialSetup.RearBias = DefDifferentialSetup.mRearBias;

	const PxReal LengthScale = 100.f; // Convert default from m to cm

	PxVehicleEngineData DefEngineData;
	EngineSetup.MOI = DefEngineData.mMOI * LengthScale * LengthScale;
	EngineSetup.PeakTorque = 1000 * LengthScale * LengthScale;
	EngineSetup.MaxOmega = 1000;
	EngineSetup.DampingRateFullThrottle = 0.5f * LengthScale * LengthScale;
	EngineSetup.DampingRateZeroThrottleClutchEngaged = 2.0f * LengthScale * LengthScale;
	EngineSetup.DampingRateZeroThrottleClutchDisengaged = 0.5f * LengthScale * LengthScale;

	PxVehicleClutchData DefClutchData;
	ClutchStrength = DefClutchData.mStrength * LengthScale * LengthScale; // convert from m to cm scale

	PxVehicleAckermannGeometryData DefAckermannSetup;
	AckermannAccuracy = DefAckermannSetup.mAccuracy;

	PxVehicleGearsData DefGearSetup;
	GearSetup.GearSwitchTime = DefGearSetup.mSwitchTime;
	GearSetup.ReverseGearRatio = DefGearSetup.mRatios[PxVehicleGearsData::eREVERSE];
	for (uint32 i = PxVehicleGearsData::eFIRST; i < DefGearSetup.mNbRatios; i++)
	{
		GearSetup.ForwardGearRatios.Add(DefGearSetup.mRatios[i]);
	}
	GearSetup.RatioMultiplier =  DefGearSetup.mFinalRatio; 

	PxVehicleAutoBoxData DefAutoBoxSetup;
	for (uint32 i = PxVehicleGearsData::eFIRST; i < PxVehicleGearsData::eGEARSRATIO_COUNT; i++)
	{
		FGearUpDownRatio RatioData;
		RatioData.UpRatio = DefAutoBoxSetup.mUpRatios[i];
		RatioData.DownRatio = DefAutoBoxSetup.mUpRatios[i];
		AutoBoxSetup.ForwardGearAutoBox.Add(RatioData);
	}
	AutoBoxSetup.NeutralGearUpRatio = DefAutoBoxSetup.mUpRatios[PxVehicleGearsData::eNEUTRAL];
	AutoBoxSetup.GearAutoBoxLatency = DefAutoBoxSetup.getLatency();
	bUseGearAutoBox = true;

	MaxSteeringSpeed = 6000.0f; // editable in vehicle blueprint
	SteeringCurve.AddZeroed(4);
	SteeringCurve[0].InVal = 0.0f;
	SteeringCurve[0].OutVal = 0.75f;
	SteeringCurve[1].InVal = 0.05f;
	SteeringCurve[1].OutVal = 0.75f;
	SteeringCurve[2].InVal = 0.25f;
	SteeringCurve[2].OutVal = 0.125f;
	SteeringCurve[3].InVal = 1.0f;
	SteeringCurve[3].OutVal = 0.1f;

	WheelSetups.AddZeroed(4);
	for (int i = 0 ; i < WheelSetups.Num(); ++i)
	{
		WheelSetups[i].WheelClass = UVehicleWheel::StaticClass();
	}
#endif // WITH_PHYSX
}

#if WITH_PHYSX

static void GetVehicleDifferential4WSetup(const FVehicleDifferential4WData& Setup, PxVehicleDifferential4WData& PxSetup)
{
//	PxSetup.mType = Setup.DifferentialType; //TBD make this work again and get rid of the case-switch
	switch(Setup.DifferentialType)
	{
		case EVehicleDifferential4W::LimitedSlip_4W:
			PxSetup.mType = PxVehicleDifferential4WData::eDIFF_TYPE_LS_4WD;
			break;
		case EVehicleDifferential4W::LimitedSlip_FrontDrive:
			PxSetup.mType = PxVehicleDifferential4WData::eDIFF_TYPE_LS_FRONTWD;
			break;
		case EVehicleDifferential4W::LimitedSlip_RearDrive:
				PxSetup.mType = PxVehicleDifferential4WData::eDIFF_TYPE_LS_REARWD;
			break;
		case EVehicleDifferential4W::Open_4W:
				PxSetup.mType = PxVehicleDifferential4WData::eDIFF_TYPE_OPEN_4WD;
			break;
		case EVehicleDifferential4W::Open_FrontDrive:
				PxSetup.mType = PxVehicleDifferential4WData::eDIFF_TYPE_OPEN_FRONTWD;
			break;
		case EVehicleDifferential4W::Open_RearDrive:
				PxSetup.mType = PxVehicleDifferential4WData::eDIFF_TYPE_OPEN_REARWD;
			break;
		default:		
			break;
	}
	PxSetup.mFrontRearSplit = Setup.FrontRearSplit;
	PxSetup.mFrontLeftRightSplit = Setup.FrontLeftRightSplit;
	PxSetup.mRearLeftRightSplit = Setup.RearLeftRightSplit;
	PxSetup.mCentreBias = Setup.CentreBias;
	PxSetup.mFrontBias = Setup.FrontBias;
	PxSetup.mRearBias = Setup.RearBias;
}

static void GetVehicleEngineSetup(const FVehicleEngineData& Setup, PxVehicleEngineData& PxSetup)
{
	PxSetup.mMOI = Setup.MOI; // convert from m to cm scale
	PxSetup.mPeakTorque = Setup.PeakTorque;
	PxSetup.mMaxOmega = Setup.MaxOmega;
	PxSetup.mDampingRateFullThrottle = Setup.DampingRateFullThrottle;
	PxSetup.mDampingRateZeroThrottleClutchEngaged = Setup.DampingRateZeroThrottleClutchEngaged;
	PxSetup.mDampingRateZeroThrottleClutchDisengaged = Setup.DampingRateZeroThrottleClutchDisengaged;
}

static void GetVehicleGearSetup(const FVehicleGearData& Setup, PxVehicleGearsData& PxSetup)
{
	PxSetup.mSwitchTime = Setup.GearSwitchTime;
	PxSetup.mRatios[PxVehicleGearsData::eREVERSE] = Setup.ReverseGearRatio;
	for (int32 i = 0; i < Setup.ForwardGearRatios.Num(); i++)
	{
		PxSetup.mRatios[i + PxVehicleGearsData::eFIRST] = Setup.ForwardGearRatios[i];
	}
	PxSetup.mFinalRatio = Setup.RatioMultiplier;
}

static void GetVehicleAutoBoxSetup(const FVehicleAutoBoxData& Setup, PxVehicleAutoBoxData& PxSetup)
{
	for (uint32 i = PxVehicleGearsData::eFIRST; i < PxVehicleGearsData::eGEARSRATIO_COUNT; i++)
	{
		const FGearUpDownRatio& RatioData = Setup.ForwardGearAutoBox[i - PxVehicleGearsData::eFIRST];
		PxSetup.mUpRatios[i] = RatioData.UpRatio;
		PxSetup.mDownRatios[i] = RatioData.DownRatio;
	}
	PxSetup.mUpRatios[PxVehicleGearsData::eNEUTRAL] = Setup.NeutralGearUpRatio;
	PxSetup.setLatency(Setup.GearAutoBoxLatency);
}

void SetupDriveHelper(const UWheeledVehicleMovementComponent4W* VehicleData, const PxVehicleWheelsSimData* PWheelsSimData, PxVehicleDriveSimData4W& DriveData)
{
	PxVehicleDifferential4WData DifferentialSetup;
	GetVehicleDifferential4WSetup(VehicleData->DifferentialSetup, DifferentialSetup);
	
	DriveData.setDiffData(DifferentialSetup);

	PxVehicleEngineData EngineSetup;
	GetVehicleEngineSetup(VehicleData->EngineSetup, EngineSetup);
	DriveData.setEngineData(EngineSetup);

	PxVehicleClutchData ClutchSetup;
	ClutchSetup.mStrength = VehicleData->ClutchStrength;
	DriveData.setClutchData(ClutchSetup);

	FVector WheelCentreOffsets[4];
	WheelCentreOffsets[PxVehicleDrive4WWheelOrder::eFRONT_LEFT] = P2UVector(PWheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_LEFT));
	WheelCentreOffsets[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT] = P2UVector(PWheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_RIGHT));
	WheelCentreOffsets[PxVehicleDrive4WWheelOrder::eREAR_LEFT] = P2UVector(PWheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_LEFT));
	WheelCentreOffsets[PxVehicleDrive4WWheelOrder::eREAR_RIGHT] = P2UVector(PWheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_RIGHT));

	PxVehicleAckermannGeometryData AckermannSetup;
	AckermannSetup.mAccuracy = VehicleData->AckermannAccuracy;
	AckermannSetup.mAxleSeparation = FMath::Abs(WheelCentreOffsets[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].X - WheelCentreOffsets[PxVehicleDrive4WWheelOrder::eREAR_LEFT].X);
	AckermannSetup.mFrontWidth = FMath::Abs(WheelCentreOffsets[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT].Y - WheelCentreOffsets[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].Y);
	AckermannSetup.mRearWidth = FMath::Abs(WheelCentreOffsets[PxVehicleDrive4WWheelOrder::eREAR_RIGHT].Y - WheelCentreOffsets[PxVehicleDrive4WWheelOrder::eREAR_LEFT].Y);
	DriveData.setAckermannGeometryData(AckermannSetup);

	PxVehicleGearsData GearSetup;
	GetVehicleGearSetup(VehicleData->GearSetup, GearSetup);	
	DriveData.setGearsData(GearSetup);

	PxVehicleAutoBoxData AutoBoxSetup;
	GetVehicleAutoBoxSetup(VehicleData->AutoBoxSetup, AutoBoxSetup);
	DriveData.setAutoBoxData(AutoBoxSetup);
}

void UWheeledVehicleMovementComponent4W::SetupVehicle()
{
	if (WheelSetups.Num() != 4)
	{
		PVehicle = NULL;
		PVehicleDrive = NULL;
		return;
	}

	// Setup the chassis and wheel shapes
	SetupVehicleShapes();

	// Setup mass properties
	SetupVehicleMass();

	// Setup the wheels
	PxVehicleWheelsSimData* PWheelsSimData = PxVehicleWheelsSimData::allocate(4);
	SetupWheels(PWheelsSimData);

	// Setup drive data
	PxVehicleDriveSimData4W DriveData;
	SetupDriveHelper(this, PWheelsSimData, DriveData);

	// Create the vehicle
	PxVehicleDrive4W* PVehicleDrive4W = PxVehicleDrive4W::allocate(4);
	check(PVehicleDrive4W);

	PVehicleDrive4W->setup( GPhysXSDK, UpdatedComponent->BodyInstance.GetPxRigidDynamic(), *PWheelsSimData, DriveData, 0);
	PVehicleDrive4W->setToRestState();

	// cleanup
	PWheelsSimData->free();
	PWheelsSimData = NULL;

	// cache values
	PVehicle = PVehicleDrive4W;
	PVehicleDrive = PVehicleDrive4W;

	// MSS Using PxVehicle built-in automatic transmission for now. No reverse yet, have to fix that.
 //SetUseAutoGears(bUseGearAutoBox);
	SetUseAutoGears(true);
	//SetTargetGear(1, true);

	const int MaxSteeringSamples = FMath::Min(8, SteeringCurve.Num());
	for (int i = 0; i < 8; i++)
	{
		SteeringMap[i*2 + 0] = (i < MaxSteeringSamples) ? (SteeringCurve[i].InVal * MaxSteeringSpeed) : PX_MAX_F32;
		SteeringMap[i*2 + 1] = (i < MaxSteeringSamples) ? FMath::Clamp<float>(SteeringCurve[i].OutVal, 0.0f, 1.0f) : PX_MAX_F32;
	}
}

void UWheeledVehicleMovementComponent4W::UpdateSimulation(float DeltaTime)
{
	if (PVehicleDrive == NULL)
		return;

	PxVehicleDrive4WRawInputData RawInputData;
	RawInputData.setAnalogAccel(ThrottleInput);
	RawInputData.setAnalogSteer(SteeringInput);
	RawInputData.setAnalogBrake(BrakeInput);
	RawInputData.setAnalogHandbrake(HandbrakeInput);
	
	if (!PVehicleDrive->mDriveDynData.getUseAutoGears())
	{
		RawInputData.setGearUp(bRawGearUpInput);
		RawInputData.setGearDown(bRawGearDownInput);
	}

	PxFixedSizeLookupTable<8> SpeedSteerLookup(SteeringMap,4);
	PxVehiclePadSmoothingData SmoothData = {
		{ ThrottleInputRate.RiseRate, BrakeInputRate.RiseRate, HandbrakeInputRate.RiseRate, SteeringInputRate.RiseRate, SteeringInputRate.RiseRate },
		{ ThrottleInputRate.FallRate, BrakeInputRate.FallRate, HandbrakeInputRate.FallRate, SteeringInputRate.FallRate, SteeringInputRate.FallRate }
	};

	PxVehicleDrive4W* PVehicleDrive4W = (PxVehicleDrive4W*)PVehicleDrive;
	PxVehicleDrive4WSmoothAnalogRawInputsAndSetAnalogInputs(SmoothData, SpeedSteerLookup, RawInputData, DeltaTime, false, *PVehicleDrive4W);
}

#endif // WITH_PHYSX

float UWheeledVehicleMovementComponent4W::CalcThrottleInput()
{
	return FMath::Abs(RawThrottleInput);
}

void UWheeledVehicleMovementComponent4W::UpdateEngineSetup(const FVehicleEngineData& NewEngineSetup)
{
#if WITH_PHYSX
	if (PVehicleDrive)
	{
		PxVehicleEngineData EngineSetup;
		GetVehicleEngineSetup(NewEngineSetup, EngineSetup);

		PxVehicleDrive4W* PVehicleDrive4W = (PxVehicleDrive4W*)PVehicleDrive;
		PVehicleDrive4W->mDriveSimData.setEngineData(EngineSetup);
	}
#endif
}

void UWheeledVehicleMovementComponent4W::UpdateDifferentialSetup(const FVehicleDifferential4WData& NewDifferentialSetup)
{
#if WITH_PHYSX
	if (PVehicleDrive)
	{
		PxVehicleDifferential4WData DifferentialSetup;
		GetVehicleDifferential4WSetup(NewDifferentialSetup, DifferentialSetup);

		PxVehicleDrive4W* PVehicleDrive4W = (PxVehicleDrive4W*)PVehicleDrive;
		PVehicleDrive4W->mDriveSimData.setDiffData(DifferentialSetup);
	}
#endif
}

void UWheeledVehicleMovementComponent4W::UpdateGearSetup(const FVehicleGearData& NewGearSetup)
{
#if WITH_PHYSX
	if (PVehicleDrive)
	{
		PxVehicleGearsData GearSetup;
		GetVehicleGearSetup(NewGearSetup, GearSetup);

		PxVehicleDrive4W* PVehicleDrive4W = (PxVehicleDrive4W*)PVehicleDrive;
		PVehicleDrive4W->mDriveSimData.setGearsData(GearSetup);
	}
#endif
}

void UWheeledVehicleMovementComponent4W::UpdateAutoBoxSetup(const FVehicleAutoBoxData& NewAutoBoxSetup)
{
#if WITH_PHYSX
	if (PVehicleDrive)
	{
		PxVehicleAutoBoxData AutoBoxSetup;
		GetVehicleAutoBoxSetup(NewAutoBoxSetup, AutoBoxSetup);

		PxVehicleDrive4W* PVehicleDrive4W = (PxVehicleDrive4W*)PVehicleDrive;
		PVehicleDrive4W->mDriveSimData.setAutoBoxData(AutoBoxSetup);
	}
#endif
}
