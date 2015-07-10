// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved. 

#include "ArchVisCharacterPluginPrivatePCH.h"

UArchVisCharMovementComponent::UArchVisCharMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RotationalAcceleration = FRotator(300.f, 300.f, 0.f);
	RotationalDeceleration = FRotator(300.f, 300.f, 0.f);
	MaxRotationalVelocity = FRotator(80.f, 100.f, 0.f);
	MinPitch = -85.f;
	MaxPitch = 85.f;
	
	WalkingFriction = 4.f;
	WalkingSpeed = 165.f;
	WalkingAcceleration = 500.f;
}

void UArchVisCharMovementComponent::OnRegister()
{
	Super::OnRegister();

	// copy our simplified params to the corresponding real params
	GroundFriction = WalkingFriction;
	MaxWalkSpeed = WalkingSpeed;
	MaxAcceleration = WalkingAcceleration;
}

void UArchVisCharMovementComponent::PhysWalking(float DeltaTime, int32 Iterations)
{
	// let character do its thing for translation
	Super::PhysWalking(DeltaTime, Iterations);

	//
	// now we will handle rotation
	//

	// update yaw
	if (CurrentRotInput.Yaw == 0)
	{
		// decelerate to 0
		if (CurrentRotationalVelocity.Yaw > 0.f)
		{
			CurrentRotationalVelocity.Yaw -= RotationalDeceleration.Yaw * DeltaTime;
			CurrentRotationalVelocity.Yaw = FMath::Max(CurrentRotationalVelocity.Yaw, 0.f);		// don't go below 0 because that would be re-accelerating the other way
		}
		else
		{
			CurrentRotationalVelocity.Yaw += RotationalDeceleration.Yaw * DeltaTime;
			CurrentRotationalVelocity.Yaw = FMath::Min(CurrentRotationalVelocity.Yaw, 0.f);		// don't go above 0 because that would be re-accelerating the other way
		}
	}
	else
	{
		// accelerate in desired direction
		float MaxYawVelMagnitude = FMath::Abs(MaxRotationalVelocity.Yaw * CurrentRotInput.Yaw);
		float const CurrentYawAccel = CurrentRotInput.Yaw * RotationalAcceleration.Yaw;
		CurrentRotationalVelocity.Yaw += CurrentYawAccel * DeltaTime;
		CurrentRotationalVelocity.Yaw = FMath::Clamp(CurrentRotationalVelocity.Yaw, -MaxYawVelMagnitude, MaxYawVelMagnitude);
	}

	// update pitch
	if (CurrentRotInput.Pitch == 0)
	{
		// decelerate to 0
		if (CurrentRotationalVelocity.Pitch > 0.f)
		{
			CurrentRotationalVelocity.Pitch -= RotationalDeceleration.Pitch * DeltaTime;
			CurrentRotationalVelocity.Pitch = FMath::Max(CurrentRotationalVelocity.Pitch, 0.f);		// don't go below 0 because that would be reaccelerating the other way
		}
		else
		{
			CurrentRotationalVelocity.Pitch += RotationalDeceleration.Pitch * DeltaTime;
			CurrentRotationalVelocity.Pitch = FMath::Min(CurrentRotationalVelocity.Pitch, 0.f);		// don't go above 0 because that would be reaccelerating the other way
		}
	}
	else
	{
		// accelerate in desired direction
		float MaxPitchVelMagnitude = FMath::Abs(MaxRotationalVelocity.Pitch * CurrentRotInput.Pitch);
		float const CurrentPitchAccel = CurrentRotInput.Pitch * RotationalAcceleration.Pitch;
		CurrentRotationalVelocity.Pitch += CurrentPitchAccel * DeltaTime;
		CurrentRotationalVelocity.Pitch = FMath::Clamp(CurrentRotationalVelocity.Pitch, -MaxPitchVelMagnitude, MaxPitchVelMagnitude);
	}

	// apply rotation
	FRotator RotDelta = CurrentRotationalVelocity * DeltaTime;
	if (!RotDelta.IsNearlyZero())
	{
		FRotator const NewRot = UpdatedComponent->GetComponentRotation() + RotDelta;

		FHitResult Hit(1.f);
		SafeMoveUpdatedComponent(FVector::ZeroVector, NewRot, false, Hit);
	}

	// consume input
	CurrentRotInput = FRotator::ZeroRotator;
}


void UArchVisCharMovementComponent::AddRotInput(float Pitch, float Yaw, float Roll)
{
	CurrentRotInput.Roll += Roll;
	CurrentRotInput.Pitch += Pitch;
	CurrentRotInput.Yaw += Yaw;
}