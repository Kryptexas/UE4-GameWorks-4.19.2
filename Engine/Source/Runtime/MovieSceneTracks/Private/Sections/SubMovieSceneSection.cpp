// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "SubMovieSceneSection.h"
#include "IMovieScenePlayer.h"
#include "MovieSceneSequence.h"


USubMovieSceneSection::USubMovieSceneSection( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{ }


void USubMovieSceneSection::SetMovieSceneAnimation( UMovieSceneSequence* InSequence )
{
	Sequence = InSequence;
}


UMovieSceneSequence* USubMovieSceneSection::GetMovieSceneAnimation() const
{
	return Sequence;
}
