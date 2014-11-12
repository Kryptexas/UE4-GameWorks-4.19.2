// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/////////////////////////////////////////////////////
// USoundCueGraph

#include "UnrealEd.h"
#include "SoundDefinitions.h"
#include "Sound/SoundCue.h"

USoundCueGraph::USoundCueGraph(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

USoundCue* USoundCueGraph::GetSoundCue() const
{
	return CastChecked<USoundCue>(GetOuter());
}
