// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*
 * Component to handle the vehicle simulation for an actor
 */

#pragma once
#include "VehicleWheel.generated.h"

UCLASS(BlueprintType, Blueprintable)
class ENGINE_API UVehicleWheel : public UObject
{
	GENERATED_UCLASS_BODY()

	// Static mesh with collision setup for wheel, will be used to create wheel shape
	// (if empty, sphere will be added as wheel shape, check bDontCreateShape flag)
	UPROPERTY(EditDefaultsOnly, Category=Shape)
	class UStaticMesh*								CollisionMesh;

	// If set, shape won't be created, but mapped from chassis mesh
	UPROPERTY(EditDefaultsOnly, Category=Shape)
	bool											bDontCreateShape;

	// If BoneName is specified, offset the wheel from the bone's location.
	// Otherwise this offsets the wheel from the vehicle's origin.
	// (X=Forward, Y=Right, Z=Up)
	UPROPERTY(EditAnywhere, Category=Wheel)
	FVector											Offset;

	// Radius of the wheel
	UPROPERTY(EditAnywhere, Category=Wheel)
	float											ShapeRadius;

	// Width of the wheel
	UPROPERTY(EditAnywhere, Category=Wheel)
	float											ShapeWidth;

	// If true, ShapeRadius and ShapeWidth will be used to automatically scale collision taken from CollisionMesh to match wheel size.
	// If false, size of CollisionMesh won't be changed. Use if you want to scale wheels manually.
	UPROPERTY(EditAnywhere, Category=Shape)
	bool											bAutoAdjustCollisionSize;

	// How strongly this wheel opposes acceleration
	UPROPERTY(EditAnywhere, Category=Wheel)
	float											Inertia;

	// Damping rate for how quickly the wheel slows down on its own
	//UPROPERTY(EditAnywhere, Category=Wheel)
	//float											DampingRate;

	// Whether handbrake should affect this wheel
	UPROPERTY(EditAnywhere, Category=Wheel)
	bool											bAffectedByHandbrake;

	// Tire type for the wheel. Determines friction
	UPROPERTY(EditAnywhere, Category=Tire)
	class UTireType*								TireType;

	// Max normalized tire load at which the tire can deliver no more lateral stiffness no matter how much extra load is applied to the tire.
	UPROPERTY(EditAnywhere, Category=Tire, meta=(ClampMin = "0.01", UIMin = "0.01"))
	float											LatStiffMaxLoad;

	// How much lateral stiffness to have given lateral slip
	UPROPERTY(EditAnywhere, Category=Tire, meta=(ClampMin = "0.01", UIMin = "0.01"))
	float											LatStiffValue;

	// Multiplier for the output lateral tire forces
	UPROPERTY(EditAnywhere, Category=Tire)
	float											LatStiffFactor;

	// How much longitudinal stiffness to have given longitudinal slip
	UPROPERTY(EditAnywhere, Category=Tire)
	float											LongStiffValue;

	// Vertical offset from vehicle center of mass where suspension forces are applied
	UPROPERTY(EditAnywhere, Category=Suspension)
	float											SuspensionForceOffset;

	// How far the wheel can drop below the resting position
	UPROPERTY(EditAnywhere, Category=Suspension)
	float											SuspensionMaxDrop;

	// How far the wheel can go above the resting position
	UPROPERTY(EditAnywhere, Category=Suspension)
	float											SuspensionMaxRaise;

	// Oscillation frequency of suspension. Standard cars have values between 5 and 10
	UPROPERTY(EditAnywhere, Category=Suspension)
	float											SuspensionNaturalFrequency;

	// The rate at which energy is dissipated from the spring. Standard cars have values between 0.8 and 1.2.
	// values < 1 are more sluggish, values > 1 or more twitchy
	UPROPERTY(EditAnywhere, Category=Suspension)
	float											SuspensionDampingRatio;

	// The vehicle that owns us
	UPROPERTY(transient)
	class UWheeledVehicleMovementComponent*								VehicleSim;

	// Our index in the vehicle's (and setup's) wheels array
	UPROPERTY(transient)
	int32											WheelIndex;

	// Longitudinal slip experienced by the wheel
	UPROPERTY(transient)
	float											DebugLongSlip;

	// Lateral slip experienced by the wheel
	UPROPERTY(transient)
	float											DebugLatSlip;

	// How much force the tire experiences at rest devided by how much force it is experiencing now
	UPROPERTY(transient)
	float											DebugNormalizedTireLoad;

	// Wheel torque
	UPROPERTY(transient)
	float											DebugWheelTorque;

	// Longitudinal force the wheel is applying to the chassis
	UPROPERTY(transient)
	float											DebugLongForce;

	// Lateral force the wheel is applying to the chassis
	UPROPERTY(transient)
	float											DebugLatForce;

	// Worldspace location of this wheel
	UPROPERTY(transient)
	FVector											Location;

	// Worldspace location of this wheel last frame
	UPROPERTY(transient)
	FVector											OldLocation;

	// Current velocity of the wheel center (change in location over time)
	UPROPERTY(transient)
	FVector											Velocity;

	UFUNCTION(BlueprintCallable, Category="Game|Components|WheeledVehicleMovement")
	float GetSteerAngle();

	UFUNCTION(BlueprintCallable, Category="Game|Components|WheeledVehicleMovement")
	float GetRotationAngle();

	UFUNCTION(BlueprintCallable, Category="Game|Components|WheeledVehicleMovement")
	float GetSuspensionOffset();

#if WITH_PHYSX

	// Our wheelshape
	physx::PxShape*									WheelShape;

	/**
	 * Initialize this wheel instance
	 */
	virtual void Init( class UWheeledVehicleMovementComponent* InVehicleSim, int32 InWheelIndex );

	/**
	 * Notify this wheel it will be removed from the scene
	 */
	virtual void Shutdown();

	/**
	 * Get the wheel setup we were created from
	 */
	struct FWheelSetup& GetWheelSetup();

	/**
	 * Tick this wheel when the vehicle ticks
	 */
	virtual void Tick( float DeltaTime );

#if WITH_EDITOR

	/**
	 * Respond to a property change in editor
	 */
	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent ) OVERRIDE;

#endif //WITH_EDITOR

protected:

	/**
	 * Get the wheel's location in physics land
	 */
	FVector GetPhysicsLocation();

#endif // WITH_PHYSX

public:

	/** Get contact surface material */
	UPhysicalMaterial* GetContactSurfaceMaterial();
};
