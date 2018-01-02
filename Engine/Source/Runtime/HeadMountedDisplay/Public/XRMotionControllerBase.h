// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMotionController.h"

enum class EControllerHand : uint8;

/**
* Base utility class for implementations of the IMotionController interface
*/
class HEADMOUNTEDDISPLAY_API FXRMotionControllerBase : public IMotionController
{
public:
	virtual ~FXRMotionControllerBase() {};

	// Begin IMotionController interface
	virtual bool GetControllerOrientationAndPosition(const int32 ControllerIndex, const FName MotionSource, FRotator& OutOrientation, FVector& OutPosition, float WorldToMetersScale) const;
	virtual ETrackingStatus GetControllerTrackingStatus(const int32 ControllerIndex, const FName MotionSource) const;
	virtual void EnumerateSources(TArray<FMotionControllerSource>& SourcesOut) const;
	virtual float GetCustomParameterValue(const FName MotionSource, FName ParameterName, bool& bValueFound) const { bValueFound = false;  return 0.f; }
	// End IMotionController interface

	// explicit (hand) source names
	static FName LeftHandSourceId;
	static FName RightHandSourceId;
	static bool GetHandEnumForSourceName(const FName Source, EControllerHand& OutHand);

	// Original GetControllerOrientationAndPosition signature for backwards compatibility 
	virtual bool GetControllerOrientationAndPosition(const int32 ControllerIndex, const EControllerHand DeviceHand, FRotator& OutOrientation, FVector& OutPosition, float WorldToMetersScale) const = 0;
	// Original GetControllerTrackingStatus signature for backwards compatibility
	virtual ETrackingStatus GetControllerTrackingStatus(const int32 ControllerIndex, const EControllerHand DeviceHand) const = 0;
};