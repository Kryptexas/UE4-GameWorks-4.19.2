// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "HeadMountedDisplay.h"

UHeadMountedDisplayFunctionLibrary::UHeadMountedDisplayFunctionLibrary(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

bool UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled()
{
	return GEngine->HMDDevice.IsValid() && GEngine->IsStereoscopic3D();
}

void UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(FRotator& DeviceRotation, FVector& DevicePosition)
{
	if(GEngine->HMDDevice.IsValid() && GEngine->IsStereoscopic3D())
	{
		FQuat OrientationAsQuat;
		FVector Position(0.f);

		GEngine->HMDDevice->GetCurrentOrientationAndPosition(OrientationAsQuat, Position);

		DeviceRotation = OrientationAsQuat.Rotator();
		DevicePosition = Position;
	}
	else
	{
		DeviceRotation = FRotator::ZeroRotator;
		DevicePosition = FVector::ZeroVector;
	}
}

bool UHeadMountedDisplayFunctionLibrary::HasValidTrackingPosition()
{
	if(GEngine->HMDDevice.IsValid() && GEngine->IsStereoscopic3D())
	{
		return GEngine->HMDDevice->HasValidTrackingPosition();
	}

	return false;
}

void UHeadMountedDisplayFunctionLibrary::GetPositionalTrackingCameraParameters(FVector& CameraOrigin, FRotator& CameraOrientation, float& HFOV, float& VFOV, float& CameraDistance, float& NearPlane, float&FarPlane)
{
	if (GEngine->HMDDevice.IsValid() && GEngine->IsStereoscopic3D() && GEngine->HMDDevice->DoesSupportPositionalTracking())
	{
		GEngine->HMDDevice->GetPositionalTrackingCameraProperties(CameraOrigin, CameraOrientation, HFOV, VFOV, CameraDistance, NearPlane, FarPlane);
	}
	else
	{
		// No HMD, zero the values
		CameraOrigin = FVector::ZeroVector;
		CameraOrientation = FRotator::ZeroRotator;
		HFOV = VFOV = 0.f;
		NearPlane = 0.f;
		FarPlane = 0.f;
		CameraDistance = 0.f;
	}
}

bool UHeadMountedDisplayFunctionLibrary::IsInLowPersistenceMode()
{
	if (GEngine->HMDDevice.IsValid() && GEngine->IsStereoscopic3D())
	{
		return GEngine->HMDDevice->IsInLowPersistenceMode();
	}
	else
	{
		return false;
	}
}

void UHeadMountedDisplayFunctionLibrary::EnableLowPersistenceMode(bool Enable)
{
	if (GEngine->HMDDevice.IsValid() && GEngine->IsStereoscopic3D())
	{
		GEngine->HMDDevice->EnableLowPersistenceMode(Enable);
	}
}

void UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition(float Yaw)
{
	if (GEngine->HMDDevice.IsValid() && GEngine->IsStereoscopic3D())
	{
		GEngine->HMDDevice->ResetOrientationAndPosition(Yaw);
	}
}
