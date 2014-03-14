// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

 
/** 
 * A node to play sounds sequentially
 * WARNING: these are not seamless
 */

#pragma once
#include "SoundNodeConcatenator.generated.h"

UCLASS(hidecategories=Object, editinlinenew, MinimalAPI, meta=( DisplayName="Concatenator" ))
class USoundNodeConcatenator : public USoundNode
{
	GENERATED_UCLASS_BODY()

	/** A volume for each input.  Automatically sized. */
	UPROPERTY(EditAnywhere, editfixedsize, Category=Concatenator)
	TArray<float> InputVolume;

public:	
	// Begin USoundNode interface. 
	virtual bool NotifyWaveInstanceFinished( struct FWaveInstance* WaveInstance ) OVERRIDE;
	virtual float GetDuration( void ) OVERRIDE;
	virtual void ParseNodes( FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances ) OVERRIDE;
	virtual int32 GetMaxChildNodes() const OVERRIDE 
	{ 
		return MAX_ALLOWED_CHILD_NODES; 
	}
	virtual void CreateStartingConnectors( void ) OVERRIDE;
	virtual void InsertChildNode( int32 Index ) OVERRIDE;
	virtual void RemoveChildNode( int32 Index ) OVERRIDE;
#if WITH_EDITOR
	/** Ensure amount of inputs matches new amount of children */
	virtual void SetChildNodes(TArray<USoundNode*>& InChildNodes) OVERRIDE;
#endif
	virtual FString GetUniqueString() const OVERRIDE;
	// End USoundNode interface. 
};

