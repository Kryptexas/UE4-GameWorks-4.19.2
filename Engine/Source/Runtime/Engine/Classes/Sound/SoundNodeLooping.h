// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

 
#pragma once
#include "Sound/SoundNode.h"
#include "SoundNodeLooping.generated.h"

/** 
 * Defines how a sound loops; either indefinitely, or for a set number of times
 */
UCLASS(hidecategories=Object, editinlinenew, MinimalAPI, meta=( DisplayName="Looping" ))
class USoundNodeLooping : public USoundNode
{
	GENERATED_UCLASS_BODY()

public:	
	// Begin USoundNode interface. 
	virtual bool NotifyWaveInstanceFinished( struct FWaveInstance* WaveInstance ) override;
	virtual float MaxAudibleDistance( float CurrentMaxDistance ) override 
	{ 
		return( WORLD_MAX ); 
	}
	virtual float GetDuration( void ) override;
	virtual void ParseNodes( FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances ) override;
	virtual FString GetUniqueString() const override;
	// End USoundNode interface. 
};



