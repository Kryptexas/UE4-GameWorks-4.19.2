// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieScene3DPathSection.h"
#include "MovieScene3DPathTrack.h"
#include "IMovieScenePlayer.h"
#include "MovieScene3DPathTrackInstance.h"

UMovieScene3DPathTrack::UMovieScene3DPathTrack( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{
}
FName UMovieScene3DPathTrack::GetTrackName() const
{
	return FName("Path");
}

TSharedPtr<IMovieSceneTrackInstance> UMovieScene3DPathTrack::CreateInstance()
{
	return MakeShareable( new FMovieScene3DPathTrackInstance( *this ) ); 
}

const TArray<UMovieSceneSection*>& UMovieScene3DPathTrack::GetAllSections() const
{
	return PathSections;
}

void UMovieScene3DPathTrack::RemoveAllAnimationData()
{
}

bool UMovieScene3DPathTrack::HasSection( UMovieSceneSection* Section ) const
{
	return PathSections.Find( Section ) != INDEX_NONE;
}

void UMovieScene3DPathTrack::RemoveSection( UMovieSceneSection* Section )
{
	PathSections.Remove( Section );
}

bool UMovieScene3DPathTrack::IsEmpty() const
{
	return PathSections.Num() == 0;
}

TRange<float> UMovieScene3DPathTrack::GetSectionBoundaries() const
{
	TArray< TRange<float> > Bounds;
	for (int32 i = 0; i < PathSections.Num(); ++i)
	{
		Bounds.Add(PathSections[i]->GetRange());
	}
	return TRange<float>::Hull(Bounds);
}

void UMovieScene3DPathTrack::AddPath(float KeyTime, float PathEndTime, const FGuid& PathId)
{
	// add the section
	UMovieScene3DPathSection* NewSection = NewObject<UMovieScene3DPathSection>(this);
	NewSection->AddPath(KeyTime, PathEndTime, PathId);
	NewSection->InitialPlacement( PathSections, KeyTime, PathEndTime, SupportsMultipleRows() );

	PathSections.Add(NewSection);
}
