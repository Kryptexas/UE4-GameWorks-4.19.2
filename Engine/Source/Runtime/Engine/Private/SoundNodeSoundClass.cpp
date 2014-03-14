// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "EnginePrivate.h"
#include "SoundDefinitions.h"

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

FString USoundNodeSoundClass::GetUniqueString() const
{
	FString Unique = TEXT( "SoundClass" );

	if (SoundClassOverride)
	{
		Unique += SoundClassOverride->GetFName().ToString();
	}

	Unique += TEXT( "/" );
	return( Unique );
}
