// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "EnginePrivate.h"
#include "SoundDefinitions.h"

/*-----------------------------------------------------------------------------
	USoundNodeWaveParam implementation
-----------------------------------------------------------------------------*/
USoundNodeWaveParam::USoundNodeWaveParam(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

float USoundNodeWaveParam::GetDuration()
{
	// Since we can't know how long this node will be we say it is indefinitely looping
	return INDEFINITELY_LOOPING_DURATION;
}

void USoundNodeWaveParam::ParseNodes( FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances )
{
	USoundWave* NewWave = NULL;
	ActiveSound.GetWaveParameter( WaveParameterName, NewWave );
	if( NewWave != NULL )
	{
		NewWave->Parse( AudioDevice, NodeWaveInstanceHash, ActiveSound, ParseParams, WaveInstances );
	}
	else
	{
		// use the default node linked to us, if any
		Super::ParseNodes( AudioDevice, NodeWaveInstanceHash, ActiveSound, ParseParams, WaveInstances );
	}
}

FString USoundNodeWaveParam::GetUniqueString() const
{
	return TEXT("WaveParam/");
}
