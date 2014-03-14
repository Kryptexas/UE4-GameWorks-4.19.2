// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCoreTypesPCH.h"
#include "MovieSceneVectorSection.h"
#include "MovieSceneVectorTrack.h"
#include "IMovieScenePlayer.h"
#include "MovieSceneVectorTrackInstance.h"

UMovieSceneVectorTrack::UMovieSceneVectorTrack( const FPostConstructInitializeProperties& PCIP )
	: Super( PCIP )
{
}

UMovieSceneSection* UMovieSceneVectorTrack::CreateNewSection()
{
	return ConstructObject<UMovieSceneSection>( UMovieSceneVectorSection::StaticClass(), this );
}

TSharedPtr<IMovieSceneTrackInstance> UMovieSceneVectorTrack::CreateInstance()
{
	return MakeShareable( new FMovieSceneVectorTrackInstance( *this ) );
}

bool UMovieSceneVectorTrack::AddKeyToSection( float Time, const FVector4& Value, int32 InChannelsUsed )
{
	const UMovieSceneSection* NearestSection = MovieSceneHelpers::FindSectionAtTime( Sections, Time );
	if (!NearestSection || CastChecked<UMovieSceneVectorSection>(NearestSection)->NewKeyIsNewData(Time, Value))
	{
		UMovieSceneVectorSection* NewSection = Cast<UMovieSceneVectorSection>( FindOrAddSection( Time ) );
		// @todo Sequencer - I don't like setting the channels used here. It should only be checked here, and set on section creation
		NewSection->SetChannelsUsed(InChannelsUsed);

		NewSection->AddKey( Time, Value );

		return true;
	}
	return false;
}

bool UMovieSceneVectorTrack::AddKeyToSection( float Time, const FVector4& Value )
{
	return AddKeyToSection(Time, Value, 4);
}

bool UMovieSceneVectorTrack::AddKeyToSection( float Time, const FVector& Value )
{
	return AddKeyToSection(Time, FVector4(Value.X, Value.Y, Value.Z, 0.f), 3);
}

bool UMovieSceneVectorTrack::AddKeyToSection( float Time, const FVector2D& Value )
{
	return AddKeyToSection(Time, FVector4(Value.X, Value.Y, 0.f, 0.f), 2);
}

bool UMovieSceneVectorTrack::Eval( float Position, float LastPosition, FVector4& OutVector ) const
{
	const UMovieSceneSection* Section = MovieSceneHelpers::FindSectionAtTime( Sections, Position );

	if( Section )
	{
		OutVector = CastChecked<UMovieSceneVectorSection>( Section )->Eval( Position );
	}

	return Section != NULL;
}
