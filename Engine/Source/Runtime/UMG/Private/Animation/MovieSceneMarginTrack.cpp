// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "MovieSceneMarginSection.h"
#include "MovieSceneMarginTrack.h"
#include "IMovieScenePlayer.h"
#include "MovieSceneMarginTrackInstance.h"
#include "MovieSceneCommonHelpers.h"

UMovieSceneMarginTrack::UMovieSceneMarginTrack(const FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
}

UMovieSceneSection* UMovieSceneMarginTrack::CreateNewSection()
{
	return ConstructObject<UMovieSceneSection>( UMovieSceneMarginSection::StaticClass(), this );
}

TSharedPtr<IMovieSceneTrackInstance> UMovieSceneMarginTrack::CreateInstance()
{
	return MakeShareable( new FMovieSceneMarginTrackInstance( *this ) ); 
}


bool UMovieSceneMarginTrack::AddKeyToSection( float Time, FMargin Value )
{
	const UMovieSceneSection* NearestSection = MovieSceneHelpers::FindSectionAtTime( Sections, Time );
	if (!NearestSection || CastChecked<UMovieSceneMarginSection>(NearestSection)->NewKeyIsNewData(Time, Value))
	{
		UMovieSceneMarginSection* NewSection = CastChecked<UMovieSceneMarginSection>( FindOrAddSection( Time ) );

		NewSection->AddKey( Time, Value );

		return true;
	}
	return false;
}


bool UMovieSceneMarginTrack::Eval( float Position, float LastPosition, FMargin& OutMargin ) const
{
	const UMovieSceneSection* Section = MovieSceneHelpers::FindSectionAtTime( Sections, Position );

	if( Section )
	{
		OutMargin = CastChecked<UMovieSceneMarginSection>( Section )->Eval( Position );
	}

	return Section != NULL;
}