// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "MixedRealitySettings.generated.h"

UCLASS(config=Engine)
class UMixedRealityFrameworkSettings : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(Config)
	FString DesiredCaptureFormat;

	UPROPERTY(Config)
	float DesiredCaptureAspectRatio;

	UPROPERTY(Config)
	int32 DesiredCaptureResolution;

	UPROPERTY(Config)
	int32 DesiredCaptureDeviceTrackIndex;

	UPROPERTY(Config)
	int32 DesiredCaptureDeviceFormatIndex;

	UPROPERTY(Config, meta=(UIMin = "5.0", UIMax = "170", ClampMin = "0.001", ClampMax = "360.0"))
	float CalibratedFOVOverride;
};
