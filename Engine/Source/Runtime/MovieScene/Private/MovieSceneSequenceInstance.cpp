// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieScenePrivatePCH.h"
#include "MovieSceneSequence.h"

FMovieSceneSequenceInstance::FMovieSceneSequenceInstance(const UMovieSceneSequence& InMovieSceneSequence, FMovieSceneSequenceIDRef InSequenceID)
{
	MovieSceneSequence = &InMovieSceneSequence;
	SequenceID = InSequenceID;
}

TRange<float> FMovieSceneSequenceInstance::GetTimeRange() const
{
	return MovieSceneSequence->GetMovieScene()->GetPlaybackRange();
}
