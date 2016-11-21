// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneSpawnSection.h"


UMovieSceneSpawnSection::UMovieSceneSpawnSection(const FObjectInitializer& Init)
	: Super(Init)
{
	EvalOptions.EnableAndSetCompletionMode(EMovieSceneCompletionMode::RestoreState);
}
