// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/** 
 * Defines the parameters for an in world non looping ambient sound e.g. a car driving by
 */

#pragma once
#include "SoundNodeAmbientNonLoop.generated.h"

UCLASS(dependson=(USoundNodeAttenuation, UDistributionFloat), editinlinenew)
class UDEPRECATED_SoundNodeAmbientNonLoop : public UDEPRECATED_SoundNodeAmbient
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	float DelayMin;

	UPROPERTY()
	float DelayMax;
};



