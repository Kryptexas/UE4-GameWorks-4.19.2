// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCoreTypesPCH.h"
#include "MovieSceneColorSection.h"
#include "MovieSceneColorTrack.h"
#include "IMovieScenePlayer.h"
#include "MovieSceneColorTrackInstance.h"

UMovieSceneColorTrack::UMovieSceneColorTrack( const FPostConstructInitializeProperties& PCIP )
	: Super( PCIP )
{
}

UMovieSceneSection* UMovieSceneColorTrack::CreateNewSection()
{
	return ConstructObject<UMovieSceneSection>( UMovieSceneColorSection::StaticClass(), this );
}

TSharedPtr<IMovieSceneTrackInstance> UMovieSceneColorTrack::CreateInstance()
{
	return MakeShareable( new FMovieSceneColorTrackInstance( *this ) ); 
}


bool UMovieSceneColorTrack::AddKeyToSection( float Time, FLinearColor Value )
{
	const UMovieSceneSection* NearestSection = MovieSceneHelpers::FindSectionAtTime( Sections, Time );
	if (!NearestSection || CastChecked<UMovieSceneColorSection>(NearestSection)->NewKeyIsNewData(Time, Value))
	{
		UMovieSceneColorSection* NewSection = CastChecked<UMovieSceneColorSection>( FindOrAddSection( Time ) );

		NewSection->AddKey( Time, Value );

		return true;
	}
	return false;
}


bool UMovieSceneColorTrack::Eval( float Position, float LastPosition, FLinearColor& OutColor ) const
{
	const UMovieSceneSection* Section = MovieSceneHelpers::FindSectionAtTime( Sections, Position );

	if( Section )
	{
		OutColor = CastChecked<UMovieSceneColorSection>( Section )->Eval( Position );
	}

	return Section != NULL;
}