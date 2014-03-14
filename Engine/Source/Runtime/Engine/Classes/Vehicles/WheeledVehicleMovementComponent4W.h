// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*
 * Base VehicleSim for the 4W PhysX vehicle class
 */
#pragma once
#include "WheeledVehicleMovementComponent4W.generated.h"

UENUM()
namespace EVehicleDifferential4W
{
	enum Type
	{
		LimitedSlip_4W,
		LimitedSlip_FrontDrive,
		LimitedSlip_RearDrive,
		Open_4W,
		Open_FrontDrive,
		Open_RearDrive,
	};
}

USTRUCT()
struct FVehicleDifferential4WData
{
	GENERATED_USTRUCT_BODY()

	/** Type of differential */
	UPROPERTY(EditAnywhere, Category=Setup)
	TEnumAsByte<EVehicleDifferential4W::Type> DifferentialType;
	
	/** Ratio of torque split between front and rear (>0.5 means more to front, <0.5 means more to rear, works only with 4W type) */
	UPROPERTY(EditAnywhere, Category=Setup)
	float FrontRearSplit;

	/** Ratio of torque split between front-left and front-right (>0.5 means more to front-left, <0.5 means more to front-right, works only with 4W and LimitedSlip_FrontDrive) */
	UPROPERTY(EditAnywhere, Category=Setup)
	float FrontLeftRightSplit;

	/** Ratio of torque split between rear-left and rear-right (>0.5 means more to rear-left, <0.5 means more to rear-right, works only with 4W and LimitedSlip_RearDrive) */
	UPROPERTY(EditAnywhere, Category=Setup)
	float RearLeftRightSplit;
	
	/** Maximum allowed ratio of average front wheel rotation speed and rear wheel rotation speeds (range: 1..inf, works only with LimitedSlip_4W) */
	UPROPERTY(EditAnywhere, Category=Setup)
	float CentreBias;

	/** Maximum allowed ratio of front-left and front-right wheel rotation speeds (range: 1..inf, works only with LimitedSlip_4W, LimitedSlip_FrontDrive) */
	UPROPERTY(EditAnywhere, Category=Setup)
	float FrontBias;

	/** Maximum allowed ratio of rear-left and rear-right wheel rotation speeds (range: 1..inf, works only with LimitedSlip_4W, LimitedSlip_FrontDrive) */
	UPROPERTY(EditAnywhere, Category=Setup)
	float RearBias;
};

USTRUCT()
struct FVehicleEngineData
{
	GENERATED_USTRUCT_BODY()

	/** Moment of inertia of the engine around the axis of rotation. */
	UPROPERTY(EditAnywhere, Category=Setup)
	float MOI;

	/** Maximum torque available to apply to the engine (kg m^2 s^-2). */ 
	UPROPERTY(EditAnywhere, Category=Setup)
	float PeakTorque;

	/** Maximum rotation speed of the engine (radians per second: rad s^-1) */
	UPROPERTY(EditAnywhere, Category=Setup)
	float MaxOmega;

	/** Damping rate of engine when full throttle is applied */
	UPROPERTY(EditAnywhere, Category=Setup, AdvancedDisplay)
	float DampingRateFullThrottle;

	/** Damping rate of engine in at zero throttle when the clutch is engaged */
	UPROPERTY(EditAnywhere, Category=Setup, AdvancedDisplay)
	float DampingRateZeroThrottleClutchEngaged;

	/** Damping rate of engine in at zero throttle when the clutch is disengaged (in neutral gear) */
	UPROPERTY(EditAnywhere, Category=Setup, AdvancedDisplay)
	float DampingRateZeroThrottleClutchDisengaged;
};

USTRUCT()
struct FVehicleGearData
{
	GENERATED_USTRUCT_BODY()

	/** Time it takes to switch gear (seconds) */
	UPROPERTY(EditAnywhere, Category=Setup)
	float GearSwitchTime;

	/** Reverse gear ratio */
	UPROPERTY(EditAnywhere, Category=Setup)
	float ReverseGearRatio;

	/** Forward gear ratios (up to 30) */
	UPROPERTY(EditAnywhere, Category=Setup)
	TArray<float> ForwardGearRatios;

	/** Gear ratio multiplier */
	UPROPERTY(EditAnywhere, Category=Setup)
	float RatioMultiplier;
};

USTRUCT()
struct FGearUpDownRatio
{
	GENERATED_USTRUCT_BODY()

	/** Value of engineRevs/maxEngineRevs that is high enough to increment gear (range 0..1) */
	UPROPERTY(EditAnywhere, Category=Setup)
	float UpRatio;

	/** Value of engineRevs/maxEngineRevs that is low enough to decrement gear (range 0..1) */
	UPROPERTY(EditAnywhere, Category=Setup)
	float DownRatio;
};

USTRUCT()
struct FVehicleAutoBoxData
{
	GENERATED_USTRUCT_BODY()

	/** Value of engineRevs/maxEngineRevs for auto gear change (range 0..1) */ 
	UPROPERTY(EditAnywhere, Category=Setup, EditFixedSize)
	TArray<FGearUpDownRatio> ForwardGearAutoBox;

	/** Value of engineRevs/maxEngineRevs that is high enough to increment gear (range 0..1) */ 
	UPROPERTY(EditAnywhere, Category=Setup)
	float NeutralGearUpRatio;

	/** Latency time is the minimum time that must pass between each gear change that is initiated by the autobox */ 
	UPROPERTY(EditAnywhere, Category=Setup)
	float GearAutoBoxLatency;
};

UCLASS(HeaderGroup=Component)
class ENGINE_API UWheeledVehicleMovementComponent4W : public UWheeledVehicleMovementComponent
{
	GENERATED_UCLASS_BODY()

	/** Engine */
	UPROPERTY(EditAnywhere, Category=VehicleSetup)
	FVehicleEngineData EngineSetup;

	/** Differential */
	UPROPERTY(EditAnywhere, Category=VehicleSetup, AdvancedDisplay)
	FVehicleDifferential4WData DifferentialSetup;

	/** Gears */
	UPROPERTY(EditAnywhere, Category=VehicleSetup, AdvancedDisplay)
	FVehicleGearData GearSetup;
	
	/** Automatic gear switching */
	UPROPERTY(EditAnywhere, Category=VehicleSetup, AdvancedDisplay)
	FVehicleAutoBoxData AutoBoxSetup;

	/** Should use auto gear switching by default? */
	UPROPERTY(EditAnywhere, Category=VehicleSetup)
	bool bUseGearAutoBox;

	/** Strength of clutch */
	UPROPERTY(EditAnywhere, Category=VehicleSetup, AdvancedDisplay)
	float ClutchStrength;

	/** Accuracy of Ackermann steer calculation (range: 0..1) */
	UPROPERTY(EditAnywhere, Category=VehicleSetup, AdvancedDisplay)
	float AckermannAccuracy;

	/** Max speed value in steering curve */
	UPROPERTY(EditAnywhere, Category=VehicleSetup)
	float MaxSteeringSpeed;

	/** Steering strength defined for speed in normalized range [0 .. 1] */
	UPROPERTY(EditAnywhere, Category=VehicleSetup, AdvancedDisplay)
	TArray<FFloatPair> SteeringCurve;

protected:

#if WITH_PHYSX

	float SteeringMap[8*2];

	/** Allocate and setup the PhysX vehicle */
	virtual void SetupVehicle() OVERRIDE;

	virtual void UpdateSimulation(float DeltaTime) OVERRIDE;

#endif // WITH_PHYSX

	/** Compute throttle input */
	virtual float CalcThrottleInput() OVERRIDE;

	/** update simulation data: engine */
	void UpdateEngineSetup(const FVehicleEngineData& NewEngineSetup);

	/** update simulation data: differential */
	void UpdateDifferentialSetup(const FVehicleDifferential4WData& NewDifferentialSetup);

	/** update simulation data: gears */
	void UpdateGearSetup(const FVehicleGearData& NewGearSetup);

	/** update simulation data: auto box */
	void UpdateAutoBoxSetup(const FVehicleAutoBoxData& NewAutoBoxSetup);
};

