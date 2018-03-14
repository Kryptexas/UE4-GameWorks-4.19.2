//
// Copyright (C) Google Inc. 2017. All rights reserved.
//

#pragma once

#include "ResonanceAudioCommon.h"
#include "ResonanceAudioSettings.generated.h"

UCLASS(config = Engine, defaultconfig)
class RESONANCEAUDIO_API UResonanceAudioSettings : public UObject
{
	GENERATED_BODY()

public:

	UResonanceAudioSettings();

	// Global Quality mode to use for directional sound sources.
	UPROPERTY(GlobalConfig, EditAnywhere, Category = General)
	ERaQualityMode QualityMode;

	// Default settings for global reverb: This is overridden when a player enters Audio Volumes.
	UPROPERTY(GlobalConfig, EditAnywhere, Category = General, meta = (AllowedClasses = "ResonanceAudioReverbPluginPreset"))
	FSoftObjectPath GlobalReverbPreset;
};
