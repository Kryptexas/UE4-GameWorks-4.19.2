// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

 
#pragma once
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
	virtual bool NotifyWaveInstanceFinished( struct FWaveInstance* WaveInstance ) OVERRIDE;
	virtual float MaxAudibleDistance( float CurrentMaxDistance ) OVERRIDE 
	{ 
		return( WORLD_MAX ); 
	}
	virtual float GetDuration( void ) OVERRIDE;
	virtual void ParseNodes( FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances ) OVERRIDE;
	virtual FString GetUniqueString() const OVERRIDE;
	// End USoundNode interface. 
};



