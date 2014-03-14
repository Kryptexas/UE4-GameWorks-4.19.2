// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "HeadMountedDisplayFunctionLibrary.generated.h"

UCLASS()
class UHeadMountedDisplayFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	/**
	 * Returns whether or not we are currently using the head mounted display
	 */
	UFUNCTION(BlueprintPure, Category="Input|HeadMountedDisplay")
	static bool IsHeadMountedDisplayEnabled();

	/**
	 * Grabs the current orientation and position for the HMD.  If positional tracking is not available, DevicePosition will be a zero vector
	 *
	 * @param DeviceRotation	(out) The device's current rotation
	 * @param DevicePosition	(out) The device's current position, in its own tracking space
	 */
	UFUNCTION(BlueprintPure, Category="Input|HeadMountedDisplay")
	static void GetOrientationAndPosition(FRotator& DeviceRotation, FVector& DevicePosition);

	/**
	 * If the HMD supports positional tracking, whether or not we are currently being tracked	
	 */
 	UFUNCTION(BlueprintPure, Category="Input|HeadMountedDisplay")
	static bool HasValidTrackingPosition();

	/**
	 * If the HMD has a positional tracking camera, this will return the game-world location of the camera, as well as the parameters for the bounding region of tracking.
	 * This allows an in-game representation of the legal positional tracking range.  All values will be zeroed if the camera is not available or the HMD does not support it.
	 *
	 * @param CameraOrigin		(out) Origin, in world-space, of the tracking camera
	 * @param CameraOrientation (out) Rotation, in world-space, of the tracking camera
	 * @param HFOV				(out) Field-of-view, horizontal, in degrees, of the valid tracking zone of the camera
	 * @param VFOV				(out) Field-of-view, vertical, in degrees, of the valid tracking zone of the camera
	 * @param CameraDistance	(out) Nominal distance to camera, in world-space
	 * @param NearPlane			(out) Near plane distance of the tracking volume, in world-space
	 * @param FarPlane			(out) Far plane distance of the tracking volume, in world-space
	 */
	UFUNCTION(BlueprintPure, Category="Input|HeadMountedDisplay")
	static void GetPositionalTrackingCameraParameters(FVector& CameraOrigin, FRotator& CameraOrientation, float& HFOV, float& VFOV, float& CameraDistance, float& NearPlane, float&FarPlane);

	/**
	 * Returns true, if HMD is in low persistence mode. 'false' otherwise.
	 */
	UFUNCTION(BlueprintPure, Category="Input|HeadMountedDisplay")
	static bool IsInLowPersistenceMode();

	/**
	 * Switches between low and full persistence modes.
	 *
	 * @param Enable			(in) 'true' to enable low persistence mode; 'false' otherwise
	 */
	UFUNCTION(BlueprintCallable, Category="Input|HeadMountedDisplay")
	static void EnableLowPersistenceMode(bool Enable);

	/** 
	 * Resets orientation by setting roll and pitch to 0, assuming that current yaw is forward direction and assuming
	 * current position as a 'zero-point' (for positional tracking). 
	 *
	 * @param Yaw				(in) the desired yaw to be set after orientation reset.
	 */
	UFUNCTION(BlueprintCallable, Category="Input|HeadMountedDisplay")
	static void ResetOrientationAndPosition(float Yaw = 0.f);
};
