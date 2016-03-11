// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
//
#include "SteamVRPrivatePCH.h"
#include "Classes/SteamVRFunctionLibrary.h"
#include "SteamVRHMD.h"

USteamVRFunctionLibrary::USteamVRFunctionLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

FSteamVRHMD* GetSteamVRHMD()
{
	if (GEngine->HMDDevice.IsValid() && (GEngine->HMDDevice->GetHMDDeviceType() == EHMDDeviceType::DT_SteamVR))
	{
		return static_cast<FSteamVRHMD*>(GEngine->HMDDevice.Get());
	}

	return nullptr;
}

void USteamVRFunctionLibrary::GetValidTrackedDeviceIds(TEnumAsByte<ESteamVRTrackedDeviceType> DeviceType, TArray<int32>& OutTrackedDeviceIds)
{
	OutTrackedDeviceIds.Empty();

	FSteamVRHMD* SteamVRHMD = GetSteamVRHMD();
	if (SteamVRHMD)
	{
		SteamVRHMD->GetTrackedDeviceIds(DeviceType, OutTrackedDeviceIds);
	}
}

bool USteamVRFunctionLibrary::GetTrackedDevicePositionAndOrientation(int32 DeviceId, FVector& OutPosition, FRotator& OutOrientation)
{
	bool RetVal = false;

	FSteamVRHMD* SteamVRHMD = GetSteamVRHMD();
	if (SteamVRHMD)
	{
		FQuat DeviceOrientation = FQuat::Identity;
		RetVal = SteamVRHMD->GetTrackedObjectOrientationAndPosition(DeviceId, DeviceOrientation, OutPosition);
		OutOrientation = DeviceOrientation.Rotator();
	}

	return RetVal;
}

bool USteamVRFunctionLibrary::GetHandPositionAndOrientation(int32 ControllerIndex, EControllerHand Hand, FVector& OutPosition, FRotator& OutOrientation)
{
	bool RetVal = false;

	FSteamVRHMD* SteamVRHMD = GetSteamVRHMD();
	if (SteamVRHMD)
	{
		FQuat DeviceOrientation = FQuat::Identity;
		RetVal = SteamVRHMD->GetControllerHandPositionAndOrientation(ControllerIndex, Hand, OutPosition, DeviceOrientation);
		OutOrientation = DeviceOrientation.Rotator();
	}

	return RetVal;
}


int32 USteamVRFunctionLibrary::GetNumberOfLighthouses()
{
	int32 NumLighthouses = 0;

	FSteamVRHMD* SteamVRHMD = GetSteamVRHMD();
	if( SteamVRHMD )
	{
		TArray< int32 > TrackedLighthouseDeviceIds;
		SteamVRHMD->GetTrackedDeviceIds( ESteamVRTrackedDeviceType::TrackingReference, /* Out */ TrackedLighthouseDeviceIds );

		NumLighthouses = TrackedLighthouseDeviceIds.Num();
	}

	return NumLighthouses;
}


bool USteamVRFunctionLibrary::GetLighthousePositionAndOrientation( const int32 LighthouseIndex, FVector& OutPosition, FRotator& OutOrientation )
{
	bool RetVal = false;

	FSteamVRHMD* SteamVRHMD = GetSteamVRHMD();
	if( SteamVRHMD )
	{
		TArray< int32 > TrackedLighthouseDeviceIds;
		SteamVRHMD->GetTrackedDeviceIds( ESteamVRTrackedDeviceType::TrackingReference, /* Out */ TrackedLighthouseDeviceIds );

		if( LighthouseIndex >= 0 && LighthouseIndex < TrackedLighthouseDeviceIds.Num() )
		{
			FQuat DeviceOrientation = FQuat::Identity;
			RetVal = SteamVRHMD->GetTrackedObjectOrientationAndPosition( TrackedLighthouseDeviceIds[ LighthouseIndex ], DeviceOrientation, OutPosition);
			OutOrientation = DeviceOrientation.Rotator();
		}
	}

	return RetVal;
}

void USteamVRFunctionLibrary::SetTrackingSpace(TEnumAsByte<ESteamVRTrackingSpace> NewSpace)
{
	FSteamVRHMD* SteamVRHMD = GetSteamVRHMD();
	if (SteamVRHMD)
	{
		SteamVRHMD->SetTrackingSpace(NewSpace);
	}
}

TEnumAsByte<ESteamVRTrackingSpace> USteamVRFunctionLibrary::GetTrackingSpace()
{
	ESteamVRTrackingSpace RetVal = ESteamVRTrackingSpace::Standing;

	FSteamVRHMD* SteamVRHMD = GetSteamVRHMD();
	if (SteamVRHMD)
	{
		RetVal = SteamVRHMD->GetTrackingSpace();
	}

	return RetVal;
}

TArray<FVector> USteamVRFunctionLibrary::GetSoftBounds()
{
	TArray<FVector> SoftBounds;

	FSteamVRHMD* SteamVRHMD = GetSteamVRHMD();
	if( SteamVRHMD )
	{
		SoftBounds = SteamVRHMD->GetBounds();
	}
	
	return SoftBounds;
}


TArray<FVector> USteamVRFunctionLibrary::GetHardBounds()
{
	TArray<FVector> HardBounds;

	FSteamVRHMD* SteamVRHMD = GetSteamVRHMD();
	if( SteamVRHMD )
	{
		HardBounds = SteamVRHMD->GetBounds();
	}

	return HardBounds;
}
