// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieScene3DPathSection.h"
#include "MovieScene3DPathTrack.h"
#include "IMovieScenePlayer.h"
#include "MovieScene3DPathTrackInstance.h"


UMovieScene3DPathTrack::UMovieScene3DPathTrack( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{ }


FName UMovieScene3DPathTrack::GetTrackName() const
{
	return FName("Path");
}


TSharedPtr<IMovieSceneTrackInstance> UMovieScene3DPathTrack::CreateInstance()
{
	return MakeShareable( new FMovieScene3DPathTrackInstance( *this ) ); 
}


void UMovieScene3DPathTrack::AddConstraint(float KeyTime, float ConstraintEndTime, const FGuid& ConstraintId)
{
	UMovieScene3DPathSection* NewSection = NewObject<UMovieScene3DPathSection>(this);
	{
		NewSection->AddPath(KeyTime, ConstraintEndTime, ConstraintId);
		NewSection->InitialPlacement( ConstraintSections, KeyTime, ConstraintEndTime, SupportsMultipleRows() );
	}

	ConstraintSections.Add(NewSection);
}
