// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MixedRealitySettings.h"

UMixedRealityFrameworkSettings::UMixedRealityFrameworkSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, DesiredCaptureAspectRatio(1.777777778f)
	, DesiredCaptureResolution(1080)
	, DesiredCaptureDeviceTrackIndex(INDEX_NONE)
	, DesiredCaptureDeviceFormatIndex(INDEX_NONE)
{}
