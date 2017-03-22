// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IAudioExtensionPlugin.h"
#include "PhononCommon.h"
#include "PhononOcclusionSourceSettings.generated.h"

UCLASS()
class PHONON_API UPhononOcclusionSourceSettings : public UOcclusionPluginSourceSettingsBase
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SourceEffect")
	EIplDirectOcclusionMethod DirectOcclusionMethod;

	// TODO: enabled only for volumetric
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SourceEffect")
	float DirectOcclusionSourceRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SourceEffect")
	bool DirectAttenuation;
};

