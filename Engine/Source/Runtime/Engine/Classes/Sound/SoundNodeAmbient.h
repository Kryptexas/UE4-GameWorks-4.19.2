// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

 
/** 
 * Defines the parameters for an in world looping ambient sound e.g. a wind sound
 */

#pragma once
#include "SoundNodeAmbient.generated.h"

USTRUCT()
struct FAmbientSoundSlot
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	class UDEPRECATED_SoundNodeWave* Wave_DEPRECATED;

	UPROPERTY()
	class USoundWave* SoundWave;

	UPROPERTY()
	float PitchScale;

	UPROPERTY()
	float VolumeScale;

	UPROPERTY()
	float Weight;

		FAmbientSoundSlot()
			: Wave_DEPRECATED(NULL)
			, SoundWave(NULL)
			, PitchScale(1.f)
			, VolumeScale(1.f)
			, Weight(1.f)
		{
		}
};

UCLASS(dependson=USoundNodeAttenuation, editinlinenew, MinimalAPI)
class UDEPRECATED_SoundNodeAmbient : public UDEPRECATED_SoundNodeDeprecated
{
	GENERATED_UCLASS_BODY()

	/* The settings for attenuating. */
	UPROPERTY()
	uint32 bAttenuate:1;

	UPROPERTY()
	uint32 bSpatialize:1;

	UPROPERTY()
	float dBAttenuationAtMax;

	/** What kind of attenuation model to use */
	UPROPERTY()
	TEnumAsByte<enum ESoundDistanceModel> DistanceModel;

	UPROPERTY()
	float RadiusMin;

	UPROPERTY()
	float RadiusMax;

	/* The settings for attenuating with a low pass filter. */
	UPROPERTY()
	uint32 bAttenuateWithLPF:1;

	UPROPERTY()
	float LPFRadiusMin;

	UPROPERTY()
	float LPFRadiusMax;

	UPROPERTY()
	float PitchMin;

	UPROPERTY()
	float PitchMax;

	UPROPERTY()
	float VolumeMin;

	UPROPERTY()
	float VolumeMax;

	UPROPERTY()
	TArray<struct FAmbientSoundSlot> SoundSlots;

};



