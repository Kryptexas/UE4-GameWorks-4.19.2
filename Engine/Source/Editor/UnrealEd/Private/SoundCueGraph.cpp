// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/////////////////////////////////////////////////////
// USoundCueGraph

#include "UnrealEd.h"
#include "SoundDefinitions.h"

USoundCueGraph::USoundCueGraph(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

USoundCue* USoundCueGraph::GetSoundCue() const
{
	return CastChecked<USoundCue>(GetOuter());
}
