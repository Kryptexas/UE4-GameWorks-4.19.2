// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneVisibilityTrack.h"
#include "IMovieScenePlayer.h"
#include "MovieSceneVisibilitySection.h"
#include "MovieSceneVisibilityTrackInstance.h"


UMovieSceneVisibilityTrack::UMovieSceneVisibilityTrack( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{ }


UMovieSceneSection* UMovieSceneVisibilityTrack::CreateNewSection()
{
	return NewObject<UMovieSceneSection>(this, UMovieSceneVisibilitySection::StaticClass(), NAME_None, RF_Transactional);
}


TSharedPtr<IMovieSceneTrackInstance> UMovieSceneVisibilityTrack::CreateInstance()
{
	return MakeShareable( new FMovieSceneVisibilityTrackInstance( *this ) );
}


bool UMovieSceneVisibilityTrack::AddKeyToSection( float Time, bool Value, FKeyParams KeyParams )
{
	const UMovieSceneSection* NearestSection = MovieSceneHelpers::FindNearestSectionAtTime( Sections, Time );
	if (!NearestSection || KeyParams.bAddKeyEvenIfUnchanged || CastChecked<UMovieSceneVisibilitySection>(NearestSection)->NewKeyIsNewData(Time, Value, KeyParams))
	{
		Modify();

		UMovieSceneVisibilitySection* NewSection = CastChecked<UMovieSceneVisibilitySection>(FindOrAddSection(  Time ));
	
		NewSection->AddKey( Time, Value, KeyParams );

		return true;
	}
	return false;
}

bool UMovieSceneVisibilityTrack::CanKeyTrack(float Time, bool Value, FKeyParams KeyParams) const
{
	// The property that's being changed is bHiddenInGame. But we want to store the inverse of that as visibility, so invert the incoming value.
	return Super::CanKeyTrack(Time, !Value, KeyParams);
}