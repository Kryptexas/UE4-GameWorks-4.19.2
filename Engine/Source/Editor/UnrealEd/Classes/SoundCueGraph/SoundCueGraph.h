// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "SoundCueGraph.generated.h"

UCLASS(MinimalAPI)
class USoundCueGraph : public UEdGraph
{
	GENERATED_UCLASS_BODY()

	/** Returns the SoundCue that contains this graph */
	UNREALED_API class USoundCue* GetSoundCue() const;
};

