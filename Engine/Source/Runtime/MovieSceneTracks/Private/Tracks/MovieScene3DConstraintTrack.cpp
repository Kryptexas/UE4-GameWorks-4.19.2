// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieScene3DConstraintSection.h"
#include "MovieScene3DConstraintTrack.h"
#include "IMovieScenePlayer.h"
#include "MovieScene3DConstraintTrackInstance.h"


UMovieScene3DConstraintTrack::UMovieScene3DConstraintTrack( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{ }


const TArray<UMovieSceneSection*>& UMovieScene3DConstraintTrack::GetAllSections() const
{
	return ConstraintSections;
}


void UMovieScene3DConstraintTrack::RemoveAllAnimationData()
{
	// do nothing
}


bool UMovieScene3DConstraintTrack::HasSection( UMovieSceneSection* Section ) const
{
	return ConstraintSections.Find( Section ) != INDEX_NONE;
}


void UMovieScene3DConstraintTrack::AddSection( UMovieSceneSection* Section )
{
	ConstraintSections.Add( Section );
}


void UMovieScene3DConstraintTrack::RemoveSection( UMovieSceneSection* Section )
{
	ConstraintSections.Remove( Section );
}


bool UMovieScene3DConstraintTrack::IsEmpty() const
{
	return ConstraintSections.Num() == 0;
}


TRange<float> UMovieScene3DConstraintTrack::GetSectionBoundaries() const
{
	TArray< TRange<float> > Bounds;

	for (int32 i = 0; i < ConstraintSections.Num(); ++i)
	{
		Bounds.Add(ConstraintSections[i]->GetRange());
	}

	return TRange<float>::Hull(Bounds);
}
