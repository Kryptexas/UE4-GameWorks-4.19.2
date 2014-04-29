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

	PxVehicleEngineData DefEngineData;
	EngineSetup.MOI = DefEngineData.mMOI;
	EngineSetup.PeakTorque = DefEngineData.mPeakTorque;
	EngineSetup.MaxRPM = OmegaToRPM(DefEngineData.mMaxOmega);
	EngineSetup.DampingRateFullThrottle = DefEngineData.mDampingRateFullThrottle;
	EngineSetup.DampingRateZeroThrottleClutchEngaged = DefEngineData.mDampingRateZeroThrottleClutchEngaged;
	EngineSetup.DampingRateZeroThrottleClutchDisengaged = DefEngineData.mDampingRateZeroThrottleClutchDisengaged;

	PxVehicleClutchData DefClutchData;
	TransmissionSetup.ClutchStrength = DefClutchData.mStrength;

	PxVehicleAckermannGeometryData DefAckermannSetup;
	AckermannAccuracy = DefAckermannSetup.mAccuracy;

	PxVehicleGearsData DefGearSetup;
	TransmissionSetup.GearSwitchTime = DefGearSetup.mSwitchTime;
	TransmissionSetup.ReverseGearRatio = DefGearSetup.mRatios[PxVehicleGearsData::eREVERSE];
	TransmissionSetup.FinalRatio = DefGearSetup.mFinalRatio;

	PxVehicleAutoBoxData DefAutoBoxSetup;
	TransmissionSetup.NeutralGearUpRatio = DefAutoBoxSetup.mUpRatios[PxVehicleGearsData::eNEUTRAL];
	TransmissionSetup.GearAutoBoxLatency = DefAutoBoxSetup.getLatency();
	TransmissionSetup.bUseGearAutoBox = true;

	for (uint32 i = PxVehicleGearsData::eFIRST; i < DefGearSetup.mNbRatios; i++)
	{
		FVehicleGearData GearData;
		GearData.DownRatio = DefAutoBoxSetup.mDownRatios[i];
		GearData.UpRatio = DefAutoBoxSetup.mUpRatios[i];
		GearData.Ratio = DefGearSetup.mRatios[i];
		TransmissionSetup.ForwardGears.Add(GearData);
	}
	
	MaxSteeringSpeed = 100.f; // editable in vehicle blueprint
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

#if WITH_EDITOR
void UWheeledVehicleMovementComponent4W::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == TEXT("DownRatio"))
	{
		for (int32 GearIdx = 0; GearIdx < TransmissionSetup.ForwardGears.Num(); ++GearIdx)
		{
			FVehicleGearData & GearData = TransmissionSetup.ForwardGears[GearIdx];
			GearData.DownRatio = FMath::Min(GearData.DownRatio, GearData.UpRatio);
		}
	}
	else if (PropertyName == TEXT("UpRatio"))
	{
		for (int32 GearIdx = 0; GearIdx < TransmissionSetup.ForwardGears.Num(); ++GearIdx)
		{
			FVehicleGearData & GearData = TransmissionSetup.ForwardGears[GearIdx];
			GearData.UpRatio = FMath::Max(GearData.DownRatio, GearData.UpRatio);
		}
	}
}
#endif

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
	PxSetup.mMOI = M2ToCm2(Setup.MOI);
	PxSetup.mPeakTorque = M2ToCm2(Setup.PeakTorque);	//convert Nm to (kg cm^2/s^2)
	PxSetup.mMaxOmega = RPMToOmega(Setup.MaxRPM);
	PxSetup.mDampingRateFullThrottle = M2ToCm2(Setup.DampingRateFullThrottle);
	PxSetup.mDampingRateZeroThrottleClutchEngaged = M2ToCm2(Setup.DampingRateZeroThrottleClutchEngaged);
	PxSetup.mDampingRateZeroThrottleClutchDisengaged = M2ToCm2(Setup.DampingRateZeroThrottleClutchDisengaged);
}

static void GetVehicleGearSetup(const FVehicleTransmissionData& Setup, PxVehicleGearsData& PxSetup)
{
	PxSetup.mSwitchTime = Setup.GearSwitchTime;
	PxSetup.mRatios[PxVehicleGearsData::eREVERSE] = Setup.ReverseGearRatio;
	for (int32 i = 0; i < Setup.ForwardGears.Num(); i++)
	{
		PxSetup.mRatios[i + PxVehicleGearsData::eFIRST] = Setup.ForwardGears[i].Ratio;
	}
	PxSetup.mFinalRatio = Setup.FinalRatio;
	PxSetup.mNbRatios = Setup.ForwardGears.Num() + PxVehicleGearsData::eFIRST;
}

static void GetVehicleAutoBoxSetup(const FVehicleTransmissionData& Setup, PxVehicleAutoBoxData& PxSetup)
{
	for (int32 i = 0; i < Setup.ForwardGears.Num(); i++)
	{
		const FVehicleGearData& GearData = Setup.ForwardGears[i];
		PxSetup.mUpRatios[i] = GearData.UpRatio;
		PxSetup.mDownRatios[i] = GearData.DownRatio;
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
	ClutchSetup.mStrength = M2ToCm2(VehicleData->TransmissionSetup.ClutchStrength);
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
	GetVehicleGearSetup(VehicleData->TransmissionSetup, GearSetup);	
	DriveData.setGearsData(GearSetup);

	PxVehicleAutoBoxData AutoBoxSetup;
	GetVehicleAutoBoxSetup(VehicleData->TransmissionSetup, AutoBoxSetup);
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
		SteeringMap[i*2 + 0] = (i < MaxSteeringSamples) ? (SteeringCurve[i].InVal * KmHToCmS(MaxSteeringSpeed)) : PX_MAX_F32;
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

void UWheeledVehicleMovementComponent4W::UpdateEngineSetup(const FVehicleEngineData& NewEngineSetup)
{
#if WITH_PHYSX
	if (PVehicleDrive)
	{
		PxVehicleEngineData EngineData;
		GetVehicleEngineSetup(NewEngineSetup, EngineData);

		PxVehicleDrive4W* PVehicleDrive4W = (PxVehicleDrive4W*)PVehicleDrive;
		PVehicleDrive4W->mDriveSimData.setEngineData(EngineData);
	}
#endif
}

void UWheeledVehicleMovementComponent4W::UpdateDifferentialSetup(const FVehicleDifferential4WData& NewDifferentialSetup)
{
#if WITH_PHYSX
	if (PVehicleDrive)
	{
		PxVehicleDifferential4WData DifferentialData;
		GetVehicleDifferential4WSetup(NewDifferentialSetup, DifferentialData);

		PxVehicleDrive4W* PVehicleDrive4W = (PxVehicleDrive4W*)PVehicleDrive;
		PVehicleDrive4W->mDriveSimData.setDiffData(DifferentialData);
	}
#endif
}

void UWheeledVehicleMovementComponent4W::UpdateTransmissionSetup(const FVehicleTransmissionData& NewTransmissionSetup)
{
#if WITH_PHYSX
	if (PVehicleDrive)
	{
		PxVehicleGearsData GearData;
		GetVehicleGearSetup(NewTransmissionSetup, GearData);

		PxVehicleAutoBoxData AutoBoxData;
		GetVehicleAutoBoxSetup(NewTransmissionSetup, AutoBoxData);

		PxVehicleDrive4W* PVehicleDrive4W = (PxVehicleDrive4W*)PVehicleDrive;
		PVehicleDrive4W->mDriveSimData.setGearsData(GearData);
		PVehicleDrive4W->mDriveSimData.setAutoBoxData(AutoBoxData);
	}
#endif
}

void BackwardsConvertCm2ToM2(float & val, float defaultValue)
{
	if (val != defaultValue)
	{
		val = Cm2ToM2(val);
	}
}

void UWheeledVehicleMovementComponent4W::Serialize(FArchive & Ar)
{
	Super::Serialize(Ar);
#if WITH_PHYSX
	if (Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_VEHICLES_UNIT_CHANGE)
	{
		PxVehicleEngineData DefEngineData;
		float DefaultRPM = OmegaToRPM(DefEngineData.mMaxOmega);
		
		//we need to convert from old units to new. This backwards compatable code fails in the rare case that they were using very strange values that are the new defaults in the correct units.
		BackwardsConvertCm2ToM2(EngineSetup.PeakTorque, DefEngineData.mPeakTorque);
		EngineSetup.MaxRPM = EngineSetup.MaxRPM != DefaultRPM ? OmegaToRPM(EngineSetup.MaxRPM) : DefaultRPM;	//need to convert from rad/s to RPM
		MaxSteeringSpeed = MaxSteeringSpeed != 100.f ? CmSToKmH(MaxSteeringSpeed) : 100.f;	//we now store as km/h instead of cm/s
	}

	if (Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_VEHICLES_UNIT_CHANGE2)
	{
		PxVehicleEngineData DefEngineData;
		PxVehicleClutchData DefClutchData;
		float DefaultRPM = OmegaToRPM(DefEngineData.mMaxOmega);

		//we need to convert from old units to new. This backwards compatable code fails in the rare case that they were using very strange values that are the new defaults in the correct units.
		BackwardsConvertCm2ToM2(EngineSetup.DampingRateFullThrottle, DefEngineData.mDampingRateFullThrottle);
		BackwardsConvertCm2ToM2(EngineSetup.DampingRateZeroThrottleClutchDisengaged, DefEngineData.mDampingRateZeroThrottleClutchDisengaged);
		BackwardsConvertCm2ToM2(EngineSetup.DampingRateZeroThrottleClutchEngaged, DefEngineData.mDampingRateZeroThrottleClutchEngaged);
		BackwardsConvertCm2ToM2(EngineSetup.MOI, DefEngineData.mMOI);
		BackwardsConvertCm2ToM2(TransmissionSetup.ClutchStrength, DefClutchData.mStrength);
	}
#endif
}

void UWheeledVehicleMovementComponent4W::ComputeConstants()
{
	Super::ComputeConstants();
	MaxEngineRPM = EngineSetup.MaxRPM;
}

