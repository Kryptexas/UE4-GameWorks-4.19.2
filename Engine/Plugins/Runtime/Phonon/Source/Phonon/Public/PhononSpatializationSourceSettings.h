// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IAudioExtensionPlugin.h"
#include "PhononCommon.h"
#include "PhononSpatializationSourceSettings.generated.h"

UCLASS()
class PHONON_API UPhononSpatializationSourceSettings : public USpatializationPluginSourceSettingsBase
{
	GENERATED_BODY()

public:

	UPROPERTY(GlobalConfig, EditAnywhere, Category = SourceEffect)
	EIplSpatializationMethod SpatializationMethod;

	UPROPERTY(GlobalConfig, EditAnywhere, Category = SourceEffect, meta = (DisplayName = "HRTF Interpolation Method"))
	EIplHrtfInterpolationMethod HrtfInterpolationMethod;
};

