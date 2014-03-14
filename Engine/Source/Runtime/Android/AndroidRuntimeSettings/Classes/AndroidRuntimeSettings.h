// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AndroidRuntimeSettings.generated.h"

UENUM()
namespace EAndroidScreenOrientation
{
	enum Type
	{
		// Portrait orientation (the display is taller than it is wide)
		Portrait UMETA(ManifestValue = "portrait"),

		// Portrait orientation rotated 180 degrees
		ReversePortrait UMETA(ManifestValue = "reversePortrait"),

		// Use either portrait or reverse portrait orientation, based on the device orientation sensor
		SensorPortrait UMETA(ManifestValue = "sensorPortrait"),

		// Landscape orientation (the display is wider than it is tall)
		Landscape UMETA(ManifestValue = "landscape"),

		// Landscape orientation rotated 180 degrees
		ReverseLandscape UMETA(ManifestValue = "reverseLandscape"),

		// Use either landscape or reverse landscape orientation, based on the device orientation sensor
		SensorLandscape UMETA(ManifestValue = "sensorLandscape"),

		// Use any orientation the device normally supports, based on the device orientation sensor
		Sensor UMETA(ManifestValue = "sensor"),

		// Use any orientation (including ones the device wouldn't choose in Sensor mode), based on the device orientation sensor
		FullSensor UMETA(ManifestValue = "fullSensor"),
	};
}

/**
 * Implements the settings for the Android runtime platform.
 */
UCLASS(config = Engine)
class ANDROIDRUNTIMESETTINGS_API UAndroidRuntimeSettings : public UObject
{
public:
	GENERATED_UCLASS_BODY()

	// The permitted orientation or orientations of the application on the device
	UPROPERTY(GlobalConfig, EditAnywhere, Category = AppManifest)
	TEnumAsByte<EAndroidScreenOrientation::Type> Orientation;

};
