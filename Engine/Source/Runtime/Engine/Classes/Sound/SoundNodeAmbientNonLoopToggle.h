// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/** 
 * Defines the parameters for an in world non looping ambient sound e.g. a car driving by
 */

#pragma once
#include "SoundNodeAmbientNonLoopToggle.generated.h"

UCLASS(hidecategories=Object, dependson=USoundNodeAttenuation, editinlinenew)
class UDEPRECATED_SoundNodeAmbientNonLoopToggle : public UDEPRECATED_SoundNodeAmbientNonLoop
{
	GENERATED_UCLASS_BODY()
};

