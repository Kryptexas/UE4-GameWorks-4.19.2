// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "SoundNodeDialoguePlayer.generated.h"

/**
 Sound node that contains a reference to the dialogue table to pull from and be played
*/
UCLASS(hidecategories = Object, editinlinenew, MinimalAPI, meta = (DisplayName = "Dialogue Player"))
class USoundNodeDialoguePlayer : public USoundNode
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=DialoguePlayer)
	FDialogueWaveParameter DialogueWaveParameter;

	/* Whether the dialogue line should be played looping */
	UPROPERTY(EditAnywhere, Category=DialoguePlayer)
	uint32 bLooping:1;

public:	
	// Begin USoundNode Interface
	virtual int32 GetMaxChildNodes() const OVERRIDE;
	virtual float GetDuration() OVERRIDE;
	virtual void ParseNodes( FAudioDevice* AudioDevice, const UPTRINT NodeWaveInstanceHash, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams, TArray<FWaveInstance*>& WaveInstances ) OVERRIDE;
	virtual FString GetUniqueString() const OVERRIDE;
	// End USoundNode Interface

};

