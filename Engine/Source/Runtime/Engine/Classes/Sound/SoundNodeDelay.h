// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "SoundNodeDelay.generated.h"

/** 
 * Defines a delay
 */
UCLASS(hidecategories=Object, editinlinenew, MinimalAPI, meta=( DisplayName="Delay" ))
class USoundNodeDelay : public USoundNode
{
	GENERATED_UCLASS_BODY()

	/* The lower bound of delay time in seconds. */
	UPROPERTY(EditAnywhere, Category=Delay )
	float DelayMin;

	/* The upper bound of delay time in seconds. */
	UPROPERTY(EditAnywhere, Category=Delay )
	float DelayMax;

public:
	// Begin USoundNode interface. 
	virtual void ParseNodes( FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances ) OVERRIDE;
	virtual float GetDuration( void ) OVERRIDE;
	virtual FString GetUniqueString() const OVERRIDE;
	// End USoundNode interface. 
};



