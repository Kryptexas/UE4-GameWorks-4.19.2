// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

 
#pragma once
#include "SoundNodeSoundClass.generated.h"

/** 
 * Remaps the SoundClass of SoundWaves underneath this
 */
UCLASS(hidecategories=Object, editinlinenew, MinimalAPI, meta=( DisplayName="SoundClass" ))
class USoundNodeSoundClass : public USoundNode
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=SoundClass)
	USoundClass* SoundClassOverride;

public:
	// Begin USoundNode interface. 
	virtual void ParseNodes( class FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances ) OVERRIDE;
	virtual FString GetUniqueString() const OVERRIDE;
	// End USoundNode interface. 
};
