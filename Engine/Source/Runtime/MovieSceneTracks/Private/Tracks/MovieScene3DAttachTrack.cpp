// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieScene3DAttachSection.h"
#include "MovieScene3DAttachTrack.h"
#include "IMovieScenePlayer.h"
#include "MovieScene3DAttachTrackInstance.h"


UMovieScene3DAttachTrack::UMovieScene3DAttachTrack( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{ }


FName UMovieScene3DAttachTrack::GetTrackName() const
{
	return FName("Attach");
}


TSharedPtr<IMovieSceneTrackInstance> UMovieScene3DAttachTrack::CreateInstance()
{
	return MakeShareable( new FMovieScene3DAttachTrackInstance( *this ) ); 
}


void UMovieScene3DAttachTrack::AddConstraint(float KeyTime, float ConstraintEndTime, const FGuid& ConstraintId)
{
	// add the section
	UMovieScene3DAttachSection* NewSection = NewObject<UMovieScene3DAttachSection>(this);
	NewSection->AddAttach(KeyTime, ConstraintEndTime, ConstraintId);
	NewSection->InitialPlacement( ConstraintSections, KeyTime, ConstraintEndTime, SupportsMultipleRows() );

	ConstraintSections.Add(NewSection);
}
