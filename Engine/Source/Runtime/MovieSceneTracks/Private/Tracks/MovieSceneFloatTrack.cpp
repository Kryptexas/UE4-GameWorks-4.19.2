// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

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


bool UMovieSceneFloatTrack::Eval( float Position, float LastPosition, float& InOutFloat ) const
{
	const UMovieSceneSection* Section = MovieSceneHelpers::FindNearestSectionAtTime( Sections, Position );

	if( Section )
	{
		if (!Section->IsInfinite())
		{
			Position = FMath::Clamp(Position, Section->GetStartTime(), Section->GetEndTime());
		}

		InOutFloat = CastChecked<UMovieSceneFloatSection>( Section )->Eval( Position, InOutFloat );
	}

	return Section != nullptr;
}

