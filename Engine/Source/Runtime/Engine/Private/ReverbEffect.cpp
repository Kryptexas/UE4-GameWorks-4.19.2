// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "SoundDefinitions.h"

UReverbEffect::UReverbEffect(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	Density = 1.0f;
	Diffusion = 1.0f;
	Gain = 0.32f;
	GainHF = 0.89f;
	DecayTime = 1.49f;
	DecayHFRatio = 0.83f;
	ReflectionsGain = 0.05f;
	ReflectionsDelay = 0.007f;
	LateGain = 1.26f;
	LateDelay = 0.011f;
	AirAbsorptionGainHF = 0.994f;
	RoomRolloffFactor = 0.0f;
}
