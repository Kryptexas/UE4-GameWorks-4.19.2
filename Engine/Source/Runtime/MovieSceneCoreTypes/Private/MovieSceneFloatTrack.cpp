// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCoreTypesPCH.h"
#include "MovieSceneFloatSection.h"
#include "MovieSceneFloatTrack.h"
#include "IMovieScenePlayer.h"
#include "MovieSceneFloatTrackInstance.h"


UMovieSceneFloatTrack::UMovieSceneFloatTrack( const FPostConstructInitializeProperties& PCIP )
	: Super( PCIP )
{
}

UMovieSceneSection* UMovieSceneFloatTrack::CreateNewSection()
{
	return ConstructObject<UMovieSceneSection>( UMovieSceneFloatSection::StaticClass(), this );
}

TSharedPtr<IMovieSceneTrackInstance> UMovieSceneFloatTrack::CreateInstance()
{
	return MakeShareable( new FMovieSceneFloatTrackInstance( *this ) ); 
}


bool UMovieSceneFloatTrack::AddKeyToSection( float Time, float Value )
{
	const UMovieSceneSection* NearestSection = MovieSceneHelpers::FindSectionAtTime( Sections, Time );
	if (!NearestSection || CastChecked<UMovieSceneFloatSection>(NearestSection)->NewKeyIsNewData(Time, Value))
	{
		UMovieSceneFloatSection* NewSection = Cast<UMovieSceneFloatSection>( FindOrAddSection( Time ) );

		NewSection->AddKey( Time, Value );

		return true;
	}
	return false;
}

bool UMovieSceneFloatTrack::Eval( float Position, float LastPosition, float& OutFloat ) const
{
	const UMovieSceneSection* Section = MovieSceneHelpers::FindSectionAtTime( Sections, Position );

	if( Section )
	{
		OutFloat = CastChecked<UMovieSceneFloatSection>( Section )->Eval( Position );
	}

	return Section != NULL;
}
