// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** 
 * A sound module file
 */

#include "SoundMod.generated.h"

UCLASS(hidecategories=Object, MinimalAPI, BlueprintType)
class USoundMod : public USoundBase
{
	GENERATED_UCLASS_BODY()

	/** If set, when played directly (not through a sound cue) the nid will be played looping. */
	UPROPERTY(EditAnywhere, Category=SoundMod)
	uint32 bLooping:1;

	/** The mod file data */
	FByteBulkData				RawData;

private:

	/** Memory containing the data copied from the compressed bulk data */
	uint8*	ResourceData;

public:	
	// Begin UObject interface. 
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	// End UObject interface. 

	// Begin USoundBase interface.
	virtual bool IsPlayable() const OVERRIDE;
	virtual void Parse(class FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances) OVERRIDE;
	virtual float GetMaxAudibleDistance() OVERRIDE;
	// End USoundBase interface.
};

