// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "SteamVRFunctionLibrary.generated.h"

/**
 * SteamVR Extensions Function Library
 */
UCLASS(MinimalAPI)
class USteamVRFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()
	
	/**
	 * Returns an array of the currently tracked device IDs
	 *
	 * @param	OutTrackedDeviceIds	(out) Array containing the ID of each device that's currently tracked
	 */
	UFUNCTION(BlueprintPure, Category="SteamVR")
	static void GetValidTrackedDeviceIds(TArray<int32>& OutTrackedDeviceIds);

	/**
	 * Gets the orientation and position (in device space) of the device with the specified ID
	 *
	 * @param	DeviceId		Id of the device to get tracking info for
	 * @param	OutPosition		(out) Current position of the device
	 * @param	OutOrientation	(out) Current orientation of the device
	 * @return	True if the specified device id had a valid tracking pose this frame, false otherwise
	 */
	UFUNCTION(BlueprintPure, Category = "SteamVR")
	static bool GetTrackedDevicePositionAndOrientation(int32 DeviceId, FVector& OutPosition, FRotator& OutOrientation);

	/**
	 * Given a controller index, returns the attached tracked device ID
	 *
	 * @param	ControllerIndex	Index of the controller to get the tracked device ID for
	 * @param	OutDeviceId		(out) Tracked device ID for the controller
	 * @return	True if the specified controller index has a valid tracked device ID
	 */
	UFUNCTION(BlueprintPure, Category = "SteamVR")
	static bool GetTrackedDeviceIdFromControllerIndex(int32 ControllerIndex, int32& OutDeviceId);
};
