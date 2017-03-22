// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IAudioExtensionPlugin.h"
#include "PhononCommon.h"
#include "PhononReverbSourceSettings.generated.h"

UCLASS()
class PHONON_API UPhononReverbSourceSettings : public UReverbPluginSourceSettingsBase
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SourceEffect")
	EIplSimulationType IndirectSimulationType;

	// TODO: enabled only if not using mixer for propagation effects
	/*
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SourceEffect")
	EIplIndirectSpatializationMethod IndirectSpatializationMethod;
	*/
};

