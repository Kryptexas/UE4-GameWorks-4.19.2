// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "EnginePrivate.h"
#include "SoundDefinitions.h"

/*-----------------------------------------------------------------------------
	USoundNodeAmbient implementation.
-----------------------------------------------------------------------------*/
UDEPRECATED_SoundNodeAmbient::UDEPRECATED_SoundNodeAmbient(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bAttenuate = true;
	bSpatialize = true;
	dBAttenuationAtMax = -60;
	RadiusMin = 2000;
	RadiusMax = 5000;
	DistanceModel = ATTENUATION_Linear;
	LPFRadiusMin = 3500;
	LPFRadiusMax = 7000;
	VolumeMin = 0.7f;
	VolumeMax = 0.7f;
	PitchMin = 1.0f;
	PitchMax = 1.0f;
}
