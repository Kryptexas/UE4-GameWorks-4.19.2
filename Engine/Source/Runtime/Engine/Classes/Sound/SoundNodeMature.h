// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

 
#pragma once
#include "SoundNodeMature.generated.h"

/**
 * This SoundNode uses UEngine::bAllowMatureLanguage to determine whether child nodes
 * that have USoundWave::bMature=true should be played. 
 */
UCLASS(hidecategories=Object, editinlinenew, MinimalAPI, meta=( DisplayName="Mature" ))
class USoundNodeMature : public USoundNode
{
	GENERATED_UCLASS_BODY()


public:
	// Begin UObject Interface
	virtual void PostLoad() OVERRIDE;
	// End UObject Interface

	// Begin USoundNode interface.
	virtual void ParseNodes( FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances ) OVERRIDE;
	virtual void CreateStartingConnectors( void ) OVERRIDE;
	virtual int32 GetMaxChildNodes() const OVERRIDE;
	virtual FString GetUniqueString() const OVERRIDE;
	// End USoundNode interface.
};

