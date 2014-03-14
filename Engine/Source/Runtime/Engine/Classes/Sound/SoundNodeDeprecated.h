// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

 
/** 
 * An intermediary class to parent all deprecated sound nodes from to easily
 * restrict them from being placed on a sound canvas
 */

#pragma once
#include "SoundNodeDeprecated.generated.h"

UCLASS(deprecated, abstract, MinimalAPI)
class UDEPRECATED_SoundNodeDeprecated : public USoundNode
{
	GENERATED_UCLASS_BODY()
};

