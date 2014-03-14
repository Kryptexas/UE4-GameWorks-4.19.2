// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

 
#pragma once
#include "SoundNodeRandom.generated.h"

/** 
 * Selects sounds from a random set
 */
UCLASS(hidecategories=Object, editinlinenew, MinimalAPI, meta=( DisplayName="Random" ))
class USoundNodeRandom : public USoundNode
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, editfixedsize, Category=Random)
	TArray<float> Weights;

	/** If greater than 0, then upon each level load such a number of inputs will be randomly selected
	 *  and the rest will be removed. This can be used to cut down the memory usage of large randomizing
	 *  cues.
	 */
	UPROPERTY(EditAnywhere, Category=Random)
	int32 PreselectAtLevelLoad;

	/** 
	 * Determines whether or not this SoundNodeRandom should randomize with or without
	 * replacement.  
	 *
	 * WithoutReplacement means that only nodes left will be valid for 
	 * selection.  So with that, you are guarenteed to have only one occurrence of the
	 * sound played until all of the other sounds in the set have all been played.
	 *
	 * WithReplacement means that a node will be chosen and then placed back into the set.
	 * So one could play the same sound over and over if the probabilities don't go your way :-)
	 */
	UPROPERTY(EditAnywhere, Category=Random)
	uint32 bRandomizeWithoutReplacement:1;

	/**
	 * Internal state of which sounds have been played.  This is only used at runtime
	 * to keep track of which sounds have been played
	 */
	UPROPERTY(transient)
	TArray<bool> HasBeenUsed;

	/** Counter var so we don't have to count all of the used sounds each time we choose a sound **/
	UPROPERTY(transient)
	int32 NumRandomUsed;


public:
	// Begin UObject Interface
	virtual void PostLoad() OVERRIDE;
	// End UObject Interface

	// Begin USoundNode interface.
	virtual void ParseNodes( FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances ) OVERRIDE;
	virtual int32 GetMaxChildNodes() const OVERRIDE 
	{ 
		return MAX_ALLOWED_CHILD_NODES; 
	}
	virtual void InsertChildNode( int32 Index ) OVERRIDE;
	virtual void RemoveChildNode( int32 Index ) OVERRIDE;
#if WITH_EDITOR
	/** Ensure Random weights and usage array matches new amount of children */
	virtual void SetChildNodes(TArray<USoundNode*>& InChildNodes) OVERRIDE;
#endif //WITH_EDITOR
	virtual void CreateStartingConnectors( void ) OVERRIDE;
	virtual FString GetUniqueString() const OVERRIDE;
	// End USoundNode interface.

	// @todo document
	void FixWeightsArray( void );

	// @todo document
	void FixHasBeenUsedArray( void );

};



