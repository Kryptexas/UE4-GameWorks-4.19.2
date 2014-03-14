// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "SoundNodeWavePlayer.generated.h"

/** 
 * Sound node that contains a reference to the raw wave file to be played
 */
UCLASS(hidecategories=Object, editinlinenew, MinimalAPI, meta=( DisplayName="Wave Player" ))
class USoundNodeWavePlayer : public USoundNode
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=WavePlayer)
	USoundWave* SoundWave;

	UPROPERTY(EditAnywhere, Category=WavePlayer)
	uint32 bLooping:1;

public:	
	// Begin USoundNode Interface
	virtual int32 GetMaxChildNodes() const OVERRIDE;
	virtual float GetDuration() OVERRIDE;
	virtual void ParseNodes( FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances ) OVERRIDE;
	virtual FString GetUniqueString() const OVERRIDE;
#if WITH_EDITOR
	virtual FString GetTitle() const OVERRIDE;
#endif
	// End USoundNode Interface

};

