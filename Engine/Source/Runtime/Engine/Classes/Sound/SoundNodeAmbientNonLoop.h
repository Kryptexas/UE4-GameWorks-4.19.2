// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/** 
 * Defines the parameters for an in world non looping ambient sound e.g. a car driving by
 */

#pragma once

#include "SoundNodeAttenuation.h"
#include "Distributions/DistributionFloat.h"
#include "SoundNodeAmbient.h"

#include "SoundNodeAmbientNonLoop.generated.h"

UCLASS(editinlinenew)
class UDEPRECATED_SoundNodeAmbientNonLoop : public UDEPRECATED_SoundNodeAmbient
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	float DelayMin;

	UPROPERTY()
	float DelayMax;
};



