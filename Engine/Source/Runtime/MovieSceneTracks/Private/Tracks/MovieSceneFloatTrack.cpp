// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneFloatSection.h"
#include "MovieSceneFloatTrack.h"
#include "IMovieScenePlayer.h"
#include "MovieSceneFloatTrackInstance.h"


UMovieSceneFloatTrack::UMovieSceneFloatTrack( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{ }


UMovieSceneSection* UMovieSceneFloatTrack::CreateNewSection()
{
	return NewObject<UMovieSceneSection>(this, UMovieSceneFloatSection::StaticClass(), NAME_None, RF_Transactional);
}


TSharedPtr<IMovieSceneTrackInstance> UMovieSceneFloatTrack::CreateInstance()
{
	return MakeShareable( new FMovieSceneFloatTrackInstance( *this ) ); 
}


bool UMovieSceneFloatTrack::AddKeyToSection( float Time, float Value, FKeyParams KeyParams )
{
	const UMovieSceneSection* NearestSection = MovieSceneHelpers::FindNearestSectionAtTime( Sections, Time );
	if (!NearestSection || KeyParams.bAddKeyEvenIfUnchanged || CastChecked<UMovieSceneFloatSection>(NearestSection)->NewKeyIsNewData(Time, Value, KeyParams))
	{
		Modify();

		UMovieSceneFloatSection* NewSection = Cast<UMovieSceneFloatSection>( FindOrAddSection( Time ) );

		NewSection->AddKey( Time, Value, KeyParams );

		return true;
	}
	return false;
}


bool UMovieSceneFloatTrack::Eval( float Position, float LastPosition, float& OutFloat ) const
{
	const UMovieSceneSection* Section = MovieSceneHelpers::FindNearestSectionAtTime( Sections, Position );

	if( Section )
	{
		if (!Section->IsInfinite())
		{
			Position = FMath::Clamp(Position, Section->GetStartTime(), Section->GetEndTime());
		}

		OutFloat = CastChecked<UMovieSceneFloatSection>( Section )->Eval( Position );
	}

	return Section != nullptr;
}

bool UMovieSceneFloatTrack::CanKeyTrack(float Time, float Value, FKeyParams KeyParams) const
{
	const UMovieSceneSection* NearestSection = MovieSceneHelpers::FindNearestSectionAtTime( Sections, Time );
	if (!NearestSection || CastChecked<UMovieSceneFloatSection>(NearestSection)->NewKeyIsNewData(Time, Value, KeyParams))
	{
		return true;
	}
	return false;
}

