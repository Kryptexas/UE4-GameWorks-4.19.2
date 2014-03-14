// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "EnginePrivate.h"
#include "SoundDefinitions.h"

/*-----------------------------------------------------------------------------
	USoundNodeModulator implementation.
-----------------------------------------------------------------------------*/
USoundNodeModulator::USoundNodeModulator(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	PitchMin = 0.95f;
	PitchMax = 1.05f;
	VolumeMin = 0.95f;
	VolumeMax = 1.05f;
}

void USoundNodeModulator::ParseNodes( FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances )
{
	RETRIEVE_SOUNDNODE_PAYLOAD( sizeof( float ) + sizeof( float ) );
	DECLARE_SOUNDNODE_ELEMENT( float, UsedVolumeModulation );
	DECLARE_SOUNDNODE_ELEMENT( float, UsedPitchModulation );

	if( *RequiresInitialization )
	{
		UsedVolumeModulation = VolumeMax + ( ( VolumeMin - VolumeMax ) * FMath::SRand() );
		UsedPitchModulation = PitchMax + ( ( PitchMin - PitchMax ) * FMath::SRand() );

		*RequiresInitialization = 0;
	}

	FSoundParseParameters UpdatedParams = ParseParams;
	UpdatedParams.Volume *= UsedVolumeModulation;
	UpdatedParams.Pitch *= UsedPitchModulation;

	Super::ParseNodes( AudioDevice, NodeWaveInstanceHash, ActiveSound, UpdatedParams, WaveInstances );
}

/**
 * Used to create a unique string to identify unique nodes
 */
FString USoundNodeModulator::GetUniqueString() const
{
	FString Unique = TEXT( "Modulator" );

	Unique += FString::Printf( TEXT( " %g %g %g %g" ), VolumeMin, VolumeMax, PitchMin, PitchMax );

	Unique += TEXT( "/" );
	return( Unique );
}
