// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "EnginePrivate.h"
#include "SoundDefinitions.h"
#include "Sound/SoundNodeSoundClass.h"

/*-----------------------------------------------------------------------------
	USoundNodeSoundClass implementation.
-----------------------------------------------------------------------------*/

USoundNodeSoundClass::USoundNodeSoundClass(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void USoundNodeSoundClass::ParseNodes( class FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances )
{
	FSoundParseParameters UpdatedParseParams = ParseParams;
	if (SoundClassOverride)
	{
		UpdatedParseParams.SoundClass = SoundClassOverride;
	}

	Super::ParseNodes( AudioDevice, NodeWaveInstanceHash, ActiveSound, UpdatedParseParams, WaveInstances );
}