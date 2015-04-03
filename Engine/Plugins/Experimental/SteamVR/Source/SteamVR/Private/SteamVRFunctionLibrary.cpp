// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
//
#include "SteamVRPrivatePCH.h"
#include "SteamVRHMD.h"
#include "Classes/SteamVRFunctionLibrary.h"

USteamVRFunctionLibrary::USteamVRFunctionLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

//@TODO steamvr:  Make these safer with device ID checking instead of nasty casting.

void USteamVRFunctionLibrary::GetValidTrackedDeviceIds(TArray<int32>& DeviceIds)
{
	DeviceIds.Empty();

	if (GEngine->HMDDevice.IsValid())
	{
		FSteamVRHMD* SteamVRHMD = static_cast<FSteamVRHMD*>(GEngine->HMDDevice.Get());
		SteamVRHMD->GetTrackedDeviceIds(DeviceIds);
	}
}

bool USteamVRFunctionLibrary::GetTrackedDevicePositionAndOrientation(int32 DeviceId, FVector& OutPosition, FRotator& OutOrientation)
{
	bool RetVal = false;

	if (GEngine->HMDDevice.IsValid())
	{
		FSteamVRHMD* SteamVRHMD = static_cast<FSteamVRHMD*>(GEngine->HMDDevice.Get());

		FQuat DeviceOrientation = FQuat::Identity;
		RetVal = SteamVRHMD->GetTrackedObjectOrientationAndPosition(DeviceId, DeviceOrientation, OutPosition);
		OutOrientation = DeviceOrientation.Rotator();
	}

	return RetVal;
}

bool USteamVRFunctionLibrary::GetTrackedDeviceIdFromControllerIndex(int32 ControllerIndex, int32& OutDeviceId)
{
	OutDeviceId = -1;

	if (!GEngine->HMDDevice.IsValid())
	{
		return false;
	}

	FSteamVRHMD* SteamVRHMD = static_cast<FSteamVRHMD*>(GEngine->HMDDevice.Get());

	return SteamVRHMD->GetTrackedDeviceIdFromControllerIndex(ControllerIndex, OutDeviceId);
}
