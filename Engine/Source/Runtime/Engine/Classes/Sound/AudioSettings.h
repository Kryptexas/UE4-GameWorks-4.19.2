// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AudioSettings.generated.h"


/**
 * Implements audio related settings.
 */
UCLASS(config=Engine, defaultconfig)
class ENGINE_API UAudioSettings
	: public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(config, EditAnywhere, Category="Audio", meta=(AllowedClasses="SoundClass", DisplayName="Default Sound Class"))
	FStringAssetReference DefaultSoundClassName;

	UPROPERTY(config, EditAnywhere, Category="Audio", meta=(AllowedClasses="SoundMix"))
	FStringAssetReference DefaultBaseSoundMix;
	
	UPROPERTY(config, EditAnywhere, Category="Audio", AdvancedDisplay, meta=(ClampMin=0.1,ClampMax=1.5))
	float LowPassFilterResonance;

	/** How many streaming sounds can be played at the same time (if more are played they will be sorted by priority) */
	UPROPERTY(config, EditAnywhere, Category="Audio", meta=(ClampMin=0))
	int32 MaximumConcurrentStreams;
};
