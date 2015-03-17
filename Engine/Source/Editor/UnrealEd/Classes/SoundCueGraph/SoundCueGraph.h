// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EdGraph/EdGraph.h"
#include "SoundCueGraph.generated.h"

UCLASS(MinimalAPI)
class USoundCueGraph : public UEdGraph
{
	GENERATED_BODY()
public:
	UNREALED_API USoundCueGraph(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Returns the SoundCue that contains this graph */
	UNREALED_API class USoundCue* GetSoundCue() const;
};

