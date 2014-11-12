// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

 
#pragma once
#include "Sound/SoundNode.h"
#include "SoundAttenuation.h"
#include "SoundNodeAttenuation.generated.h"

struct FAttenuationSettings;
class USoundAttenuation;

/** 
 * Defines how a sound's volume changes based on distance to the listener
 */
UCLASS(hidecategories=Object, editinlinenew, MinimalAPI, meta=( DisplayName="Attenuation" ))
class USoundNodeAttenuation : public USoundNode
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Attenuation, meta=(EditCondition="!bOverrideAttenuation"))
	USoundAttenuation* AttenuationSettings;

	UPROPERTY(EditAnywhere, Category=Attenuation, meta=(EditCondition="bOverrideAttenuation"))
	FAttenuationSettings AttenuationOverrides;

	UPROPERTY(EditAnywhere, Category=Attenuation)
	uint32 bOverrideAttenuation:1;

	UPROPERTY()
	uint32 bAttenuate_DEPRECATED:1;

	UPROPERTY()
	uint32 bSpatialize_DEPRECATED:1;

	UPROPERTY()
	float dBAttenuationAtMax_DEPRECATED;

	UPROPERTY()
	TEnumAsByte<enum ESoundDistanceModel> DistanceAlgorithm_DEPRECATED;

	UPROPERTY()
	TEnumAsByte<enum ESoundDistanceCalc> DistanceType_DEPRECATED;

	UPROPERTY()
	float RadiusMin_DEPRECATED;

	UPROPERTY()
	float RadiusMax_DEPRECATED;

	UPROPERTY()
	uint32 bAttenuateWithLPF_DEPRECATED:1;

	UPROPERTY()
	float LPFRadiusMin_DEPRECATED;

	UPROPERTY()
	float LPFRadiusMax_DEPRECATED;


public:
	// Begin UObject interface.
	virtual void Serialize(FArchive& Ar) override;
	// End UObject interface.

	// Begin USoundNode interface. 
	virtual void ParseNodes( class FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances ) override;
	virtual float MaxAudibleDistance( float CurrentMaxDistance ) override;
	// End USoundNode interface. 

	ENGINE_API FAttenuationSettings* GetAttenuationSettingsToApply();
};



